#include "LoadingGUI.h"
#include "System.h"
#include "Writer.h"
#include "DataModule.h"
#include "SceneLayer.h"
#include "RTEManagers.h"

#include "GUI/GUI.h"
#include "GUI/GUICollectionBox.h"
#include "GUI/GUIProgressBar.h"
#include "GUI/GUIListBox.h"
#include "GUI/GUILabel.h"
#include "GUI/AllegroScreen.h"
#include "GUI/AllegroBitmap.h"
#include "GUI/AllegroInput.h"

#include "unzip.h"
#include <corecrt_io.h>
#include <experimental/filesystem>

extern std::string g_LoadSingleModule;
extern volatile bool g_Quit;
extern bool g_LogToCLI;

namespace RTE {

	LoadingGUI g_LoadingGUI;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	AllegroScreen *m_GUIScreen = 0;
	GUIControlManager *m_LoadingGUI = 0;
	BITMAP *m_LoadingGUIBitmap = 0;
	int LoadingGUI::m_LoadingGUIPosX = 0;
	int LoadingGUI::m_LoadingGUIPosY = 0;

	AllegroInput *m_GUIInput = 0;
	Writer *m_LoadingLogWriter = 0;

	bool LoadingGUI::m_LogToCLI = false;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void LoadingGUI::InitLoadingScreen() {

		// Load the palette
		g_FrameMan.LoadPalette("Base.rte/palette.bmp");

		// Create the main GUI
		m_GUIInput = new AllegroInput(-1);
		m_GUIScreen = new AllegroScreen(g_FrameMan.GetBackBuffer32());

		// Loading splash screen
		g_FrameMan.ClearBackBuffer32();
		SceneLayer *pLoadingSplash = new SceneLayer();
		pLoadingSplash->Create(ContentFile("Base.rte/GUIs/Title/LoadingSplash.bmp"), false, Vector(), true, false, Vector(1.0, 0));

		// Hardcoded offset to make room for the loading box only if DisableLoadingScreen is false.
		if (!g_SettingsMan.DisableLoadingScreen()) {
			pLoadingSplash->SetOffset(Vector(((pLoadingSplash->GetBitmap()->w - g_FrameMan.GetResX()) / 2) + 120, 0));
		} else {
			pLoadingSplash->SetOffset(Vector(((pLoadingSplash->GetBitmap()->w - g_FrameMan.GetResX()) / 2) + 14, 0));
		}
		// Draw onto wrapped strip centered vertically on the screen
		Box splashBox(Vector(0, (g_FrameMan.GetResY() - pLoadingSplash->GetBitmap()->h) / 2), g_FrameMan.GetResX(), pLoadingSplash->GetBitmap()->h);
		pLoadingSplash->Draw(g_FrameMan.GetBackBuffer32(), splashBox);
		delete pLoadingSplash;
		pLoadingSplash = 0;

		g_FrameMan.FlipFrameBuffers();

		// Set up the loading GUI
		if (!m_LoadingGUI) {
			m_LoadingGUI = new GUIControlManager();

			// TODO: This should be using the 32bpp main menu skin, but isn't because it needs the config of the base for its listbox
			// Can get away with this hack for now because the list box that the loading menu uses displays ok when drawn on a 32bpp buffer,
			// when it's 8bpp internally, since it does not use any masked_blit calls to draw list boxes.
			// Note also how the GUIScreen passed in here has been created with an 8bpp bitmap, since that is what determines what the GUI manager uses internally
			if (!m_LoadingGUI->Create(m_GUIScreen, m_GUIInput, "Base.rte/GUIs/Skins/MainMenu", "LoadingSkin.ini")) {
				RTEAbort("Failed to create GUI Control Manager and load it from Base.rte/GUIs/Skins/MainMenu/LoadingSkin.ini");
			}
			m_LoadingGUI->Load("Base.rte/GUIs/LoadingGUI.ini");
		}

		// Place and clear the sectionProgress box
		dynamic_cast<GUICollectionBox *>(m_LoadingGUI->GetControl("root"))->SetSize(g_FrameMan.GetResX(), g_FrameMan.GetResY());
		GUIListBox *pBox = dynamic_cast<GUIListBox *>(m_LoadingGUI->GetControl("ProgressBox"));
		// Make the box a bit bigger if there's room in higher, HD resolutions
		if (g_FrameMan.GetResX() >= 960) {
			// Make the loading progress box fill the right third of the screen
			pBox->Resize((g_FrameMan.GetResX() / 3) - 12, pBox->GetHeight());
			pBox->SetPositionRel(g_FrameMan.GetResX() - pBox->GetWidth() - 12, (g_FrameMan.GetResY() / 2) - (pBox->GetHeight() / 2));
		} else {
			// Legacy positioning and sizing when running low resolutions
			pBox->SetPositionRel(g_FrameMan.GetResX() - pBox->GetWidth() - 12, (g_FrameMan.GetResY() / 2) - (pBox->GetHeight() / 2));
		}
		pBox->ClearList();

		// New mechanism to speed up loading times as it turned out that a massive amount of time is spent to update GUI control.
		if (!g_SettingsMan.DisableLoadingScreen() && !m_LoadingGUIBitmap) {
			pBox->SetVisible(false);
			m_LoadingGUIBitmap = create_bitmap_ex(8, pBox->GetWidth(), pBox->GetHeight());
			clear_to_color(m_LoadingGUIBitmap, 54);
			rect(m_LoadingGUIBitmap, 0, 0, pBox->GetWidth() - 1, pBox->GetHeight() - 1, 33);
			rect(m_LoadingGUIBitmap, 1, 1, pBox->GetWidth() - 2, pBox->GetHeight() - 2, 33);
			m_LoadingGUIPosX = pBox->GetXPos();
			m_LoadingGUIPosY = pBox->GetYPos();

		}
		// Create the loading log writer
		if (!m_LoadingLogWriter) { m_LoadingLogWriter = new Writer("LogLoading.txt"); }

		// Load all the data modules
		LoadDataModules();
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void LoadingGUI::LoadingSplashProgressReport(std::string reportString, bool newItem) {
		if (m_LogToCLI) { g_System.LogToCLI(reportString, newItem); }

		if (m_LoadingGUI) {
			g_UInputMan.Update();
			if (newItem) {
				// Write out the last line to the log file before starting a new one
				if (m_LoadingLogWriter->WriterOK()) { *m_LoadingLogWriter << reportString << "\n"; }
				// Scroll bitmap upwards
				if (m_LoadingGUIBitmap) { blit(m_LoadingGUIBitmap, m_LoadingGUIBitmap, 2, 12, 2, 2, m_LoadingGUIBitmap->w - 3, m_LoadingGUIBitmap->h - 12); }
			}
			if (m_LoadingGUIBitmap) {
				AllegroBitmap bmp(m_LoadingGUIBitmap);
				// Clear current line
				rectfill(m_LoadingGUIBitmap, 2, m_LoadingGUIBitmap->h - 12, m_LoadingGUIBitmap->w - 3, m_LoadingGUIBitmap->h - 3, 54);
				// Print new line
				g_FrameMan.GetSmallFont()->DrawAligned(&bmp, 5, m_LoadingGUIBitmap->h - 12, reportString.c_str(), GUIFont::Left);
				// DrawAligned - MaxWidth is useless here, so we're just drawing lines manually
				vline(m_LoadingGUIBitmap, m_LoadingGUIBitmap->w - 2, m_LoadingGUIBitmap->h - 12, m_LoadingGUIBitmap->h - 2, 33);
				vline(m_LoadingGUIBitmap, m_LoadingGUIBitmap->w - 1, m_LoadingGUIBitmap->h - 12, m_LoadingGUIBitmap->h - 2, 33);

				// Draw onto current frame buffer
				blit(m_LoadingGUIBitmap, g_FrameMan.GetBackBuffer32(), 0, 0, m_LoadingGUIPosX, m_LoadingGUIPosY, m_LoadingGUIBitmap->w, m_LoadingGUIBitmap->h);

				g_FrameMan.FlipFrameBuffers();
			}
			// Quit if we're commanded to during loading
			if (g_Quit) { exit(0); }
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool LoadingGUI::LoadDataModules() {

		// Clear out the PresetMan and all its DataModules
		g_PresetMan.Destroy();
		g_PresetMan.Create();

		// Unpack any ".rte.zip" files
		ExtractArchivedModules();
		// Load all unpacked modules
		g_PresetMan.LoadAllDataModules();

		return true;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void LoadingGUI::ExtractArchivedModules() {

		al_ffblk zippedModuleInfo;
		unzFile zipFile;
		for (int result = al_findfirst("*.rte.zip", &zippedModuleInfo, FA_ALL); result == 0; result = al_findnext(&zippedModuleInfo)) {

			// Report that we are attempting to unzip this thing
			LoadingSplashProgressReport("Unzipping " + std::string(zippedModuleInfo.name), true);

			// Try to open the zipped and unzip it into place as an exposed data module
			if (std::strlen(zippedModuleInfo.name) > 0 && (zipFile = unzOpen(zippedModuleInfo.name))) {
				// Go through and extract every file inside this zip, overwriting every colliding file that already exists in the install directory 

				// Get info about the zip file
				unz_global_info zipFileInfo;
				if (unzGetGlobalInfo(zipFile, &zipFileInfo) != UNZ_OK) {
					LoadingSplashProgressReport("Could not read global file info of: " + string(zippedModuleInfo.name), true);
				}

				// Buffer to hold data read from the zip file.
				char fileBuffer[s_FileBufferSize];

				// Loop to extract all files
				bool abortExtract = false;
				for (uLong i = 0; i < zipFileInfo.number_entry && !abortExtract; ++i) {
					// Get info about current file.
					unz_file_info fileInfo;
					char outputFileName[s_MaxFileName];
					if (unzGetCurrentFileInfo(zipFile, &fileInfo, outputFileName, s_MaxFileName, NULL, 0, NULL, 0) != UNZ_OK) {
						LoadingSplashProgressReport("Could not read file info of: " + std::string(outputFileName), true);
					}

					// Check if the directory we are trying to extract into exists, and if not, create it
					char outputDirName[s_MaxFileName];
					char parentDirName[s_MaxFileName];
					// Copy the file path to a separate directory path
					strcpy_s(outputDirName, sizeof(outputDirName), outputFileName);
					// Find the last slash in the directory path, so we can cut off everything after that (ie the actual filename), and only have the directory path left
					char *pSlashPos = strrchr(outputDirName, '/');
					// Try to find the other kind of slash if we found none
					if (!pSlashPos) { pSlashPos = strrchr(outputDirName, '\\'); }
					// Now that we have the slash position, terminate the directory path string right after there
					if (pSlashPos) { *(++pSlashPos) = 0; }

					// If that file's directory doesn't exist yet, then create it, and all its parent directories above if need be
					for (int nested = 0; !std::experimental::filesystem::exists(outputDirName) && pSlashPos; ++nested) {
						// Keep making new working copies of the path that we can dice up
						strcpy_s(parentDirName, sizeof(parentDirName), outputDirName[0] == '.' ? &(outputDirName[2]) : outputDirName);
						// Start off at the beginning
						pSlashPos = parentDirName;
						for (int j = 0; j <= nested && pSlashPos; ++j) {
							// Find the first slash so we can isolate the folders in the hierarchy, in descending seniority
							pSlashPos = strchr(pSlashPos, '/');
							// If we can't find any more slashes, then quit
							if (!pSlashPos) { break; }
							// If we did find a slash, go to one past it slash and try to find the next one
							pSlashPos++;
						}
						// No more nested folders to make
						if (!pSlashPos) { break; }
						// Terminate there so we are making the most senior folder
						*(pSlashPos) = 0;
						g_System.MakeDirectory(parentDirName);
					}

					// Check if this entry is a directory or file
					if (outputFileName[strlen(outputFileName) - 1] == '/' || outputFileName[strlen(outputFileName) - 1] == '\\') {
						// Entry is a directory, so create it.
						LoadingSplashProgressReport("Creating Dir: " + std::string(outputFileName), true);
						g_System.MakeDirectory(outputFileName);
						// It's a file
					} else {
						// Validate so only certain file types are extracted:  .ini .txt .lua .cfg .bmp .png .jpg .jpeg .wav .ogg .mp3
						// Get the file extension
						std::string fileExtension = std::experimental::filesystem::path(outputFileName).extension().string();
						std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
						const char *ext = fileExtension.c_str();
						// Validate only certain file types to be included! .ini .txt .lua .cfg .bmp .png .jpg .jpeg .wav .ogg .mp3
						if (!(std::strcmp(ext, ".ini") == 0 || std::strcmp(ext, ".txt") == 0 || std::strcmp(ext, ".lua") == 0 || std::strcmp(ext, ".cfg") == 0 ||
							std::strcmp(ext, ".bmp") == 0 || std::strcmp(ext, ".png") == 0 || std::strcmp(ext, ".jpg") == 0 || std::strcmp(ext, ".jpeg") == 0 ||
							std::strcmp(ext, ".wav") == 0 || std::strcmp(ext, ".ogg") == 0 || std::strcmp(ext, ".mp3") == 0)) {
							LoadingSplashProgressReport("Skipping: " + std::string(outputFileName) + " - bad extension!", true);

							// Keep going through!!
							// Close the read file within the zip archive
							unzCloseCurrentFile(zipFile);
							// Go the next entry listed in the zip file.
							if ((i + 1) < zipFileInfo.number_entry) {
								if (unzGoToNextFile(zipFile) != UNZ_OK) {
									LoadingSplashProgressReport("Could not read next file inside zip " + std::string(zippedModuleInfo.name) + " - Aborting extraction!", true);
									abortExtract = true;
									break;
								}
							}
							// Onto the next file
							continue;
						}
						// Entry is a file, so extract it.
						LoadingSplashProgressReport("Extracting: " + std::string(outputFileName), true);
						if (unzOpenCurrentFile(zipFile) != UNZ_OK) {
							LoadingSplashProgressReport("Could not open file within " + std::string(zippedModuleInfo.name), true);
						}
						// Open a file to write out the data.
						FILE *outputFile = fopen(outputFileName, "wb");
						if (outputFile == NULL) {
							LoadingSplashProgressReport("Could not open/create destination file while unzipping " + std::string(zippedModuleInfo.name), true);
						}
						// Write the entire file out, reading in buffer size chunks and spitting them out to the output stream
						int bytesRead = 0;
						int64_t totalBytesRead = 0;
						do {
							// Read a chunk
							bytesRead = unzReadCurrentFile(zipFile, fileBuffer, s_FileBufferSize);
							// Add to total tally
							totalBytesRead += bytesRead;
							// Sanity check how damn big this file we're writing is becoming.. could prevent zip bomb exploits: http://en.wikipedia.org/wiki/Zip_bomb
							if (totalBytesRead >= s_MaxUnzippedFileSize) {
								LoadingSplashProgressReport("File inside zip " + std::string(zippedModuleInfo.name) + " is turning out WAY TOO LARGE - Aborting extraction!", true);
								abortExtract = true;
								break;
							}
							// Write data to the output file
							if (bytesRead > 0) {
								fwrite(fileBuffer, bytesRead, 1, outputFile);
							} else if (bytesRead < 0) {
								LoadingSplashProgressReport("Error while reading zip " + std::string(zippedModuleInfo.name), true);
								abortExtract = true;
								break;
							}
						}
						// Keep going while bytes are still being read (0 means end of file)
						while (bytesRead > 0 && outputFile);
						// Close the output file
						fclose(outputFile);
						// Close the read file within the zip archive
						unzCloseCurrentFile(zipFile);
					}

					// Go the next entry listed in the zip file.
					if ((i + 1) < zipFileInfo.number_entry) {
						if (unzGoToNextFile(zipFile) != UNZ_OK) {
							LoadingSplashProgressReport("Could not read next file inside zip " + std::string(zippedModuleInfo.name) + " - Aborting extraction!", true);
							break;
						}
					}
				}
				// Close the zip file we've opened
				unzClose(zipFile);

				LoadingSplashProgressReport("Deleting extracted Data Module zip: " + std::string(zippedModuleInfo.name), true);
				// DELETE the zip in the install directory after decompression
				std::remove(zippedModuleInfo.name);
			} else {
				// Indicate that the unzip went awry
				LoadingSplashProgressReport("FAILED to unzip " + std::string(zippedModuleInfo.name) + " - deleting it now!", true);
				// DELETE the zip in the install directory after decompression
				// (whether successful or not - any rte.zip in the install directory is throwaway and shouldn't keep failing each load in case they do fail)
				std::remove(zippedModuleInfo.name);
			}
		}
		// Close the file search to avoid memory leaks
		al_findclose(&zippedModuleInfo);
	}
}