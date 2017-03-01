#include "oglctx.hpp"

typedef const char* ProcName;
typedef PFNGLACTIVETEXTUREPROC ProcType;

// pick an arbitrary GL function
static ProcName procName = "glActiveTexture";
static ProcType procAddr = nullptr;

template<typename TProcType> TProcType get_proc_addr(const char* proc_name) {
	#if (defined(WIN32))
	return (reinterpret_cast<TProcType>(wglGetProcAddress(reinterpret_cast<const GLubyte*>(proc_name))));
	#else
	return (reinterpret_cast<TProcType>(glXGetProcAddress(reinterpret_cast<const GLubyte*>(proc_name))));
	#endif
}



#if (defined(WIN32))

static constexpr int context_attribs[] = {WGL_CONTEXT_FLAGS, WGL_CONTEXT_DEBUG_BIT, 0};

gl::t_offscreen_context::t_offscreen_context() {
	if (procAddr == nullptr)
		procAddr = get_proc_addr<ProcType>(procName);

	// get the main (onscreen) GL rendering context
	HGLRC rc_handle = wglGetCurrentContext();

	m_rc_handle = nullptr;
	m_dc_handle = wglGetCurrentDC();

	if (m_dc_handle == nullptr || rc_handle == nullptr)
		throw (std::runtime_error("[oglctx][wglGetCurrentContext() == null || wglGetCurrentDC() == null]"));

	#ifdef WGL_ARB_create_context
	// use newer method if available
	if ((wglCreateContextAttribs != nullptr) && (m_rc_handle = wglCreateContextAttribs(m_dc_handle, rc_handle, context_attribs)) != nullptr)
		return;
	#endif

	// create a second OpenGL context within the current device context
	// while active, rendering to the default framebuffer is forbidden
	if ((m_rc_handle = wglCreateContext(m_dc_handle)) == nullptr)
		throw (std::runtime_error("[oglctx][wglCreateContext() == null]"));

	// deactivate old context
	if (!wglMakeCurrent(nullptr, nullptr))
		throw (std::runtime_error("[oglctx][!wglMakeCurrent(null, null)]"));

	// share GL resources (lists, textures, shaders, etc)
	if (!wglShareLists(rc_handle, m_rc_handle))
		throw (std::runtime_error("[oglctx][!wglShareLists(rc, m_rc)]"));

	// activate new context
	if (!wglMakeCurrent(m_dc_handle, rc_handle))
		throw (std::runtime_error("[oglctx][!wglMakeCurrent(m_dc, rc)]"));
}

gl::t_offscreen_context::~t_offscreen_context() {
	wglDeleteContext(m_rc_handle);
}


void gl::t_offscreen_context::enable() const {
	if (!wglMakeCurrent(m_dc_handle, m_rc_handle))
		throw (std::runtime_error("[oglctx::enable][!wglMakeCurrent(m_dc, m_rc)]"));

	// functions must live at the same address in both contexts
	if ((get_proc_addr<ProcType>(procName)) == procAddr)
		return;

	disable();
	throw (std::runtime_error("[oglctx::enable][ctx::procAddr != procAddr]"));
}

void gl::t_offscreen_context::disable() const {
	if (wglMakeCurrent(nullptr, nullptr))
		return;

	throw (std::runtime_error("[oglctx::disable][!wglMakeCurrent(null, null)]"));
}



#else



static constexpr int fbuffer_attribs[] = {
	GLX_RENDER_TYPE  , GLX_RGBA_BIT   ,
	GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
	GLX_BUFFER_SIZE  ,              32,
	GLX_DEPTH_SIZE   ,              24,
	GLX_STENCIL_SIZE ,               8,
	None
};

// any render-context needs a drawable pbuffer
static constexpr int pbuffer_attribs[] = {
	GLX_PBUFFER_WIDTH     ,     1,
	GLX_PBUFFER_HEIGHT    ,     1,
	GLX_PRESERVED_CONTENTS, false,
	None
};

#if 0
static constexpr int context_attribs[] = {
	GLX_CONTEXT_MAJOR_VERSION         , 3,
	GLX_CONTEXT_MINOR_VERSION         , 0,
	GLX_CONTEXT_FLAGS                 ,
	GLX_CONTEXT_DEBUG_BIT             ,
	GLX_CONTEXT_FORWARD_COMPATIBLE_BIT,
	None
};
#endif



gl::t_offscreen_context::t_offscreen_context() {
	if (procAddr == nullptr)
		procAddr = get_proc_addr<ProcType>(procName);

	// GLXDrawable current_drawable = glXGetCurrentDrawable();
	GLXContext current_context = glXGetCurrentContext();

	if (current_context == nullptr)
		throw (std::runtime_error("[oglctx][glXGetCurrentContext() == null]"));

	if ((m_display = glXGetCurrentDisplay()) == nullptr)
		throw (std::runtime_error("[oglctx][glXGetCurrentDisplay() == null]"));

	const int def_xscrn = XDefaultScreen(m_display);
	      int num_elems = 0;

	GLXFBConfig* fbuffer_cfg = glXChooseFBConfig(m_display, def_xscrn, &fbuffer_attribs[0], &num_elems);

	if (fbuffer_cfg == nullptr || num_elems == 0)
		throw (std::runtime_error("[oglctx][glXChooseFBConfig(m_display) == null]"));

	if ((m_pbuffer = glXCreatePbuffer(m_display, fbuffer_cfg[0], &pbuffer_attribs[0])) == 0)
		throw (std::runtime_error("[oglctx][glXCreatePbuffer(m_display) == 0]"));


	#if 0
	if ((m_context = glXCreateContextAttribs(m_display, fbuffer_cfg[0], current_context, true, context_attribs)) == nullptr)
		throw (std::runtime_error("[offscreen_gl_context][glXCreateContextAttribs(m_display) == null]"));

	#else

	if ((m_context = glXCreateNewContext(m_display, fbuffer_cfg[0], GLX_RGBA_TYPE, current_context, true)) == nullptr)
		throw (std::runtime_error("[oglctx][glXCreateNewContext(m_display) == null]"));
	#endif

	XFree(fbuffer_cfg);
}

gl::t_offscreen_context::~t_offscreen_context() {
	glXDestroyContext(m_display, m_context);
	glXDestroyPbuffer(m_display, m_pbuffer);
}


void gl::t_offscreen_context::enable() const {
	if (!glXMakeCurrent(m_display, m_pbuffer, m_context))
		throw (std::runtime_error("[oglctx::enable][!glXMakeCurrent(m_display, m_pbuffer, m_context)]"));

	if ((get_proc_addr<ProcType>(procName)) == procAddr)
		return;

	disable();
	throw (std::runtime_error("[oglctx::enable][ctx::procAddr != procAddr]"));
}

void gl::t_offscreen_context::disable() const {
	if (glXMakeCurrent(m_display, None, nullptr))
		return;

	throw (std::runtime_error("[oglctx::disable][!glXMakeCurrent(m_display, null, null)]"));
}

#endif

