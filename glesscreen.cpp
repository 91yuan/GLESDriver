#include "glesscreen.h"


GLESScreen::GLESScreen(int displayId)
	:QScreen(displayId)
{
	this->displayId = displayId;
	this->eglAttrs[0] = EGL_RED_SIZE;	this->eglAttrs[1] = 0;
	this->eglAttrs[2] = EGL_GREEN_SIZE;	this->eglAttrs[3] = 0;
	this->eglAttrs[4] = EGL_BLUE_SIZE; 	this->eglAttrs[5] = 0;
	this->eglAttrs[6] = EGL_ALPHA_SIZE;	this->eglAttrs[7] = 0;
	this->eglAttrs[8] = EGL_DEPTH_SIZE;	this->eglAttrs[9] = 0;
	this->eglAttrs[10] = EGL_NATIVE_VISUAL_ID;	this->eglAttrs[11] = 0;
	this->eglAttrs[12] = EGL_NONE;
}

GLESScreen::~GLESScreen(){}

bool GLESScreen::connect(const QString &displaySpec)
{
	gf_dev_info_t  gdevInfo;

	// ����������� � ���������������. � �������� ��������������� ���������� ������
	// ���������� � dev/io-display (GF_DEVICE_INDEX(0)).
	if (gf_dev_attach(&this->gfDev, GF_DEVICE_INDEX(0), &gdevInfo) != GF_ERR_OK) {
		qCritical("gf_dev_attach() failed\n");
	    return false;
	}

	// ��������, ��� ���������� � ������������ ������������� ������� �������� ����������
	if (this->displayId > gdevInfo.ndisplays - 1) {
		qCritical("No such display with id: %i\n Number of displays: %i\n",
				this->displayId, gdevInfo.ndisplays);
		return false;
	}

	// ����������� � ���������� �������. �������� ���������� � ���.
	if (gf_display_attach(&this->gfDisplay, this->gfDev, this->displayId,
			&this->displayInfo) != GF_ERR_OK) {
		qCritical("gf_display_attach() failed\n");
		return false;
	}

	// ���� �� �������� ���������� dpi
	// ������ ����������� ��������� �� �������������� ��������� ������ ��������
	const int dpi = 120;

	// ��������� ���������� �������
	this->w = displayInfo.xres;
	this->h = displayInfo.yres;
	this->dw = this->w;
	this->dh = this->h;
	this->physWidth = qRound(this->dw * 25.4 / dpi);
	this->physHeight = qRound(this->dh * 25.4 / dpi);

	// ��������� ���������� ������
	switch (this->displayInfo.format)
	{
	case GF_FORMAT_PAL8:
		this->d = 8;
		this->setPixelFormat(QImage::Format_Indexed8);
		this->setAttrs(0, 0, 0,  0, 8);
		break;
	case GF_FORMAT_PACK_ARGB1555: case GF_FORMAT_PKLE_ARGB1555:
	case GF_FORMAT_PKBE_ARGB1555:
		this->d = 16;
		this->setPixelFormat(QImage::Format_RGB555);
		this->setAttrs(5, 5, 5,  1, 16);
		break;
	case GF_FORMAT_PACK_RGB565: case GF_FORMAT_PKLE_RGB565:
	case GF_FORMAT_PKBE_RGB565:
		this->d = 16;
		this->setPixelFormat(QImage::Format_RGB16);
		this->setAttrs(5, 6, 5,  0, 16);
		break;
	case GF_FORMAT_BGR888:
		this->d = 24;
		this->pixeltype = BGRPixel;		// �������� ������� ������
		this->setPixelFormat(QImage::Format_RGB888);
		this->setAttrs(8, 8, 8,  0, 24);
		break;
	case GF_FORMAT_BGRA8888:
		this->d = 32;
		this->pixeltype = BGRPixel;
		this->setPixelFormat(QImage::Format_ARGB32);
		this->setAttrs(8, 8, 8, EGL_DONT_CARE, EGL_DONT_CARE);
		break;
	default:
		qCritical("Unknown display format: %x\n", displayInfo.format);
		return false;
	}

	return true;
}

// ������� ������������� �������� eglAttrs[] � ��������� ��������
void GLESScreen::setAttrs(int red, int green, int blue, int alpha, int depth)
{
	this->eglAttrs[1] = red;
	this->eglAttrs[3] = green;
	this->eglAttrs[5] = blue;
	this->eglAttrs[7] = alpha;
	this->eglAttrs[9] = depth;
}

bool GLESScreen::initDevice()
{
	// ��������� EGLDisplay
	this->eglDisplay = eglGetDisplay(this->gfDev);
	if (this->eglDisplay == EGL_NO_DISPLAY) {
		qCritical("eglGetDisplay() failed\n");
		return false;
	}

	// ���������� � �������� ���� �������
	if (gf_layer_attach(&this->gfLayer, this->gfDisplay,
			this->displayInfo.main_layer_index, 0) != GF_ERR_OK) {
		qCritical("gf_layer_attach() failed\n");
		return false;
	}

	// ������������� �������
	if (!eglInitialize(this->eglDisplay, NULL, NULL)) {
		qCritical("eglInitialize() failed");
		return false;
	}


	for (int i = 0; ; i++) {
	    // ��������� ��� ���� �������� ��� ����� ����
	    if (gf_layer_query(this->gfLayer, i, &this->lInfo) != GF_ERR_OK) {
	    	qCritical("Couldn't find a compatible frame "
	            "buffer configuration on layer\n");
	    	return false;
	    }

	    // ����������, ����� ������ ���� �������������� ������� ������� EGL
	    this->eglAttrs[11] = this->lInfo.format;
	    EGLint numConfigs;
	    if (eglChooseConfig(this->eglDisplay, this->eglAttrs, &this->eglConfig,
	    		1, &numConfigs) == EGL_TRUE) {
	        if (numConfigs > 0) {
	            break;
	        }
	    }
	}

	gf_surface_t    surface;
	if (gf_surface_create_layer(&surface, &this->gfLayer, 1, 0, this->w, this->h,
			this->lInfo.format, NULL, GF_SURFACE_CREATE_CPU_LINEAR_ACCESSIBLE) == GF_ERR_OK) {

		gf_layer_set_surfaces(this->gfLayer, &surface, 1);

	} else {
		qCritical("gf_surface_create_layer() failed\n");
	}

	gf_surface_info_t info;
	gf_surface_get_info(surface, &info);
	this->data = info.vaddr;
	this->lstep = info.stride;


	/*if (gf_3d_target_create(&this->target, this->gfLayer, NULL, 0,
					this->w, this->h, this->lInfo.format) != GF_ERR_OK) {
		qCritical("gf_3d_target_create() failed\n");
	    return false;
	}

	// ��������� ����.
	gf_layer_set_src_viewport(this->gfLayer, 0, 0, this->w-1, this->h-1);
	gf_layer_set_dst_viewport(this->gfLayer, 0, 0, this->w-1, this->h-1);
	gf_layer_set_filter(this->gfLayer, GF_LAYER_FILTER_NONE);
	gf_layer_enable(this->gfLayer);

	// ������� EGL window surface
	this->eglSurface = eglCreateWindowSurface(this->eglDisplay, this->eglConfig, this->target, NULL);
	if (this->eglSurface == EGL_NO_SURFACE) {
		qCritical("eglCreateWindowSurface() failed\n");
		return false;
	}

	// �������� ��������� EGL
	this->eglContext = eglCreateContext(this->eglDisplay, this->eglConfig, EGL_NO_CONTEXT, 0);
	if (this->eglContext == EGL_NO_CONTEXT) {
		 qCritical("eglCreateContext() failed");
		 return false;
	}

	if (!eglMakeCurrent(this->eglDisplay, this->eglSurface, this->eglSurface,
			this->eglContext)) {
		qCritical("eglMakeCurrent() failed");
		return false;
	}*/

	// ������������� ������������ �������
	//QScreenCursor::initSoftwareCursor();

	return true;

}

void GLESScreen::shutdownDevice()
{
	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE,
	                    EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(this->eglDisplay, this->eglContext);
	eglDestroySurface(this->eglDisplay, this->eglSurface);
	eglTerminate(this->eglDisplay);
}

void GLESScreen::disconnect()
{
	gf_display_detach(this->gfDisplay);
	gf_dev_detach(this->gfDev);
}
