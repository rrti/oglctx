#ifndef OFFSCREEN_OPENGL_CONTEXT_HDR
#define OFFSCREEN_OPENGL_CONTEXT_HDR

#include <functional>
#include <thread>

#if (defined(WIN32))
	#include <GL/wglew.h>
	#include <windows.h>
#else
	#include <GL/glew.h>
	#include <GL/glx.h>
	#include <X11/Xlib.h>
#endif



namespace gl {
	struct t_offscreen_context {
	public:
		t_offscreen_context();
		~t_offscreen_context() noexcept;

		// note: these may only be called from an offscreen thread
		void enable() const;
		void disable() const;

	private:
		t_offscreen_context(const t_offscreen_context&) = delete;
		t_offscreen_context(t_offscreen_context&&) = delete;

	private:
		#if (defined(WIN32))
		HDC m_dc_handle;
		HGLRC m_rc_handle;
		#else
		Display* m_display;

		GLXPbuffer m_pbuffer;
		GLXContext m_context;
		#endif
	};



	struct t_offscreen_thread {
	public:
		t_offscreen_thread(std::function<void()>&& render_func) { init(std::move(render_func)); }
		~t_offscreen_thread() { join(); }

	private:
		t_offscreen_thread(const t_offscreen_thread&) = delete;
		t_offscreen_thread(t_offscreen_thread&&) = delete;

		void join() { m_render_thread.join(); }
		void init(std::function<void()>&& render_func) {
			m_render_func = std::move(render_func);
			m_render_thread = std::move(std::thread(&t_offscreen_thread::render, this));
		}

		void render() {
			m_render_context.enable();
			m_render_func();
			m_render_context.disable();
		}

	private:
		std::function<void()> m_render_func;
		std::thread m_render_thread;

		t_offscreen_context m_render_context;
	};
};

#endif

