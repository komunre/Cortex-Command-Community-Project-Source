#include "SDLGUITexture.h"
#include "GUI.h"
#include "Managers/FrameMan.h"
#include "System/RTEError.h"

namespace RTE {
	SDLGUITexture::SDLGUITexture() {
		m_TextureFile.Reset();
		m_Texture = nullptr;
		m_SelfCreated = false;
		m_ClipRect.x = 0;
		m_ClipRect.y = 0;
		m_ClipRect.w = 0;
		m_ClipRect.h = 0;
	}

	SDLGUITexture::SDLGUITexture(SDL_Texture *pTexture, bool needPixelAccess) {
		m_TextureFile.Reset();
		m_Texture = pTexture;
		m_SelfCreated = false;
		m_ClipRect.x = 0;
		m_ClipRect.y = 0;
		m_ClipRect.w = 0;
		m_ClipRect.h = 0;
	}

	SDLGUITexture::~SDLGUITexture(){
		Destroy();
	}

	bool SDLGUITexture::Create(int width, int height, int depth) {
		m_SelfCreated = true;

		m_TextureFile.Reset();
		m_Texture = SDL_CreateTexture(
		    g_FrameMan.GetRenderer(), g_FrameMan.GetPixelFormat(),
		    SDL_TEXTUREACCESS_STREAMING, width, height);
		if (m_Texture == NULL) {
			return false;
		}
		return true;
	}

	bool SDLGUITexture::Create(const std::string filename) {
		m_SelfCreated = false;

		m_TextureFile.Create(filename.c_str());

		//TODO: Implement in ContentFile
		m_Texture = m_TextureFile.GetAsTexture();
		RTEAssert(m_Texture,
		          "Could not load bitmap from file into SDLTexture!");

		return true;
	}

	void SDLGUITexture::Destroy() {
		if (m_SelfCreated && m_Texture)
			SDL_DestroyTexture(m_Texture);

		m_Texture = nullptr;
	}

	void SDLGUITexture::Draw(int x, int y, GUIRect *pRect) {
		SDL_Rect destRect{pRect->x, pRect->y, m_ClipRect.w, m_ClipRect.h};
		SDL_RenderCopy(g_FrameMan.GetRenderer(), m_Texture, &m_ClipRect,
		               &destRect);
	}

	void SDLGUITexture::DrawTransScaled(GUIBitmap *pDestBitmap, int x, int y,
	                                 int width, int height) {}

	void SDLGUITexture::DrawLine(int x1, int y1, int x2, int y2,
	                          unsigned long color) {}

	void SDLGUITexture::DrawRectangle(int x, int y, int width, int height,
	                               unsigned long color, bool filled) {}

	unsigned long SDLGUITexture::GetPixel(int x, int y) {
		Uint32 *pixels = (Uint32 *)m_Pixels_ro;

		return pixels[(y * m_Pitch / 4) + x];
	}

	void SDLGUITexture::SetPixel(int x, int y, unsigned long color) {
		Uint32 *pixels = (Uint32 *)m_Pixels_wo;
		pixels[(y * m_Pitch / 4) + x] = color;
	}

	bool SDLGUITexture::LockTexture(SDL_Rect *rect) {
		RTEAssert(SDL_LockTexture(m_Texture, rect, &m_Pixels_wo, &m_Pitch) == 0,
		          "Failed to lock Texture with error: " +
		              std::string(SDL_GetError()));

		return true;
	}

	bool SDLGUITexture::UnlockTexture() {
		SDL_UnlockTexture(m_Texture);
		m_Pixels_wo = nullptr;

		m_Pitch = 0;

		return true;
	}

	void SDLGUITexture::GetClipRect(GUIRect *rect) {
		rect->x = m_ClipRect.x;
		rect->y = m_ClipRect.y;
		rect->w = m_ClipRect.x + m_ClipRect.w;
		rect->h = m_ClipRect.y + m_ClipRect.h;
	}

	void SDLGUITexture::SetClipRect(GUIRect *rect) {
		m_ClipRect.x = rect->x;
		m_ClipRect.y = rect->y;
		m_ClipRect.w = rect->w;
		m_ClipRect.h = rect->h;
	}

	void SDLGUITexture::AddClipRect(GUIRect *rect) {
		m_ClipRect.x = std::max(m_ClipRect.x, rect->x);
		m_ClipRect.y = std::max(m_ClipRect.y, rect->y);
		m_ClipRect.w =
		    std::min(m_ClipRect.w, rect->w);
		m_ClipRect.h =
		    std::min(m_ClipRect.h, rect->h);
	}
} // namespace RTE
