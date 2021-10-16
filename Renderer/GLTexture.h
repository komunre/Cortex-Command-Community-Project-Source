#ifndef _RTE_GLTEXTURE_
#define _RTE_GLTEXTURE_

#include "BlendMode.h"
#include "Surface.h"

extern "C" {
struct SDL_Surface;
}

namespace RTE {
	class Shader;
	enum class Shading {
		Base,
		Fill,
		Custom
	};

	enum class TextureAccess {
		Static,
		Straming
	};

	class RenderTarget;
	class GLTexture : public Surface {
		friend class ContentFile;

	public:
		GLTexture();
		virtual ~GLTexture();

		void render(RenderTarget *renderer, float x, float y);

		void render(RenderTarget *renderer, glm::vec2 pos);

		void render(RenderTarget *renderer, float x, float y, float angle);

		void render(RenderTarget *renderer, glm::vec2 pos, float angle);

		void render(RenderTarget *renderer, glm::vec2 pos, glm::vec2 scale);

		void render(RenderTarget *renderer, glm::vec2 pos, float angle, glm::vec2 scale);

		void render(RenderTarget *renderer, glm::vec2 pos, float angle, glm::vec2 center, glm::vec2 scale);

		void setShading(Shading shader) { m_Shading = shader; }

		Shading getShading() const { return m_Shading; }

		void setBlendMode(BlendMode blendMode) { m_BlendMode = blendMode; }
		BlendMode getBlendMode() const { return m_BlendMode; }

		void setColorMod(const glm::vec3 &colorMod) { m_ColorMod = glm::vec4(colorMod, m_ColorMod.a); }

		void setColorMod(int r, int g, int b) { m_ColorMod = glm::vec4(r / 255.0, g / 255.0, b / 255.0, m_ColorMod.a); }

		glm::vec3 getColorMod() const { return m_ColorMod.rgb(); }

		void setAlphaMod(float alphaMod) { m_ColorMod.a = alphaMod; }

		float getAlphaMod() const { return m_ColorMod.a; }

		void clearAll(uint32_t color = 0)  {}

		void Bind();

		unsigned int GetTextureID() {return m_TextureID;}

	private:
		unsigned int m_TextureID; //!< The OpenGL texture handle associated with this texture.

		glm::vec4 m_ColorMod; //!< Color multiplied in the shader stage, used for fill color as well.
		BlendMode m_BlendMode; //!< The blendmode used for drawing this texture.

		Shading m_Shading; //!< Which shader to use while drawing.
		std::shared_ptr<Shader> m_ShaderBase; //!< Base shader appropriate for the bitdepth, also used as fallback in case other shaders are unset. MUST BE SET BEFORE DRAWING.
		std::shared_ptr<Shader> m_ShaderFill; //!< Fill shader, used for drawing silhouettes, color is determined by color mod.
		std::shared_ptr<Shader> m_ShaderCustom; //!< Custom shader, TODO: user defineable?.

		/// <summary>
		/// Returns the appropriate shader for the current shading.
		/// </summary>
		/// <returns>
		/// Shared pointer to a shader object used for rendering.
		/// </returns>
		std::shared_ptr<Shader> GetCurrentShader();
	private:
		void Clear();
	};
	typedef std::shared_ptr<GLTexture> SharedTexture;
} // namespace RTE
#endif