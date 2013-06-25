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

	// Подключение к видеоустройству. В качестве видеоустройства выбирается первое
	// устройство в dev/io-display (GF_DEVICE_INDEX(0)).
	if (gf_dev_attach(&this->gfDev, GF_DEVICE_INDEX(0), &gdevInfo) != GF_ERR_OK) {
		qCritical("gf_dev_attach() failed\n");
	    return false;
	}

	// Проверка, что переданный в конструкторе идентификатор дисплея является допустимым
	if (this->displayId > gdevInfo.ndisplays - 1) {
		qCritical("No such display with id: %i\n Number of displays: %i\n",
				this->displayId, gdevInfo.ndisplays);
		return false;
	}

	// Подключение к указанному дисплею. Получеие информации о нем.
	if (gf_display_attach(&this->gfDisplay, this->gfDev, this->displayId,
			&this->displayInfo) != GF_ERR_OK) {
		qCritical("gf_display_attach() failed\n");
		return false;
	}

	// один из наиболее популярных dpi
	// выбран константным поскольку не представляется возможным узнать реальный
	const int dpi = 120;

	// установка параметров дисплея
	this->w = displayInfo.xres;
	this->h = displayInfo.yres;
	this->dw = this->w;
	this->dh = this->h;
	this->physWidth = qRound(this->dw * 25.4 / dpi);
	this->physHeight = qRound(this->dh * 25.4 / dpi);

	// Настройка параметров цветов
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
		this->pixeltype = BGRPixel;		// обратный порядок цветов
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

// Функция устанавливает атрибуты eglAttrs[] в указанные значения
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
	// Получение EGLDisplay
	this->eglDisplay = eglGetDisplay(this->gfDev);
	if (this->eglDisplay == EGL_NO_DISPLAY) {
		qCritical("eglGetDisplay() failed\n");
		return false;
	}

	// Подлючение к главному слою дисплея
	if (gf_layer_attach(&this->gfLayer, this->gfDisplay,
			this->displayInfo.main_layer_index, 0) != GF_ERR_OK) {
		qCritical("gf_layer_attach() failed\n");
		return false;
	}

	// Инициализация дисплея
	if (!eglInitialize(this->eglDisplay, NULL, NULL)) {
		qCritical("eglInitialize() failed");
		return false;
	}


	for (int i = 0; ; i++) {
	    // Проверяем все виды форматов для этого слоя
	    if (gf_layer_query(this->gfLayer, i, &this->lInfo) != GF_ERR_OK) {
	    	qCritical("Couldn't find a compatible frame "
	            "buffer configuration on layer\n");
	    	return false;
	    }

	    // Необходимо, чтобы формат слоя соответствовал формату буффера EGL
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
			this->lInfo.format, NULL, GF_SURFACE_CREATE_CPU_LINEAR_ACCESSIBLE|GF_SURFACE_PHYS_CONTIG) == GF_ERR_OK) {

		gf_layer_set_surfaces(this->gfLayer, &surface, 1);

	} else {
		qCritical("gf_surface_create_layer() failed\n");
	}

	gf_surface_info_t info;
	gf_surface_get_info(surface, &info);
	this->data = info.vaddr;
	this->lstep = info.stride;
	this->size = this->h * this->lstep;
	this->mapsize = this->h * this->lstep;

	if (gf_context_create(&this->gfContext) != GF_ERR_OK)
	{
		qCritical("gf_context_create failed\n");
		return false;
	}

	if (gf_context_set_surface(this->gfContext, surface) != GF_ERR_OK)
	{
		qCritical("gf_context_set_surface failed\n");
		return false;
	}

/*
	if (gf_3d_target_create(&this->target, this->gfLayer, NULL, 0,
					this->w, this->h, this->lInfo.format) != GF_ERR_OK) {
		qCritical("gf_3d_target_create() failed\n");
	    return false;
	}

	// Натсройки слоя.
	gf_layer_set_src_viewport(this->gfLayer, 0, 0, this->w-1, this->h-1);
	gf_layer_set_dst_viewport(this->gfLayer, 0, 0, this->w-1, this->h-1);
	gf_layer_set_filter(this->gfLayer, GF_LAYER_FILTER_NONE);
	gf_layer_enable(this->gfLayer);
*/
	// Создаем EGL window surface
	this->eglSurface = eglCreateWindowSurface(this->eglDisplay, this->eglConfig, this->target, NULL);
	if (this->eglSurface == EGL_NO_SURFACE) {
		qCritical("eglCreateWindowSurface() failed\n");
		return false;
	}

	// Создание контекста EGL
	this->eglContext = eglCreateContext(this->eglDisplay, this->eglConfig, EGL_NO_CONTEXT, 0);
	if (this->eglContext == EGL_NO_CONTEXT) {
		 qCritical("eglCreateContext() failed");
		 return false;
	}

	if (!eglMakeCurrent(this->eglDisplay, this->eglSurface, this->eglSurface,
			this->eglContext)) {
		qCritical("eglMakeCurrent() failed");
		return false;
	}

	// Инициализация программного курсора
	QScreenCursor::initSoftwareCursor();

	return true;
}

// Освобождение всех выделенных ресурсов
void GLESScreen::shutdownDevice()
{
	if (this->gfContext)
		gf_context_free(this->gfContext);

	if (this->gfLayer)
		gf_layer_detach(this->gfLayer);

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

/*
 * Переопределение метода exposeRegion из QScreen. Вызывается каждый раз, когда
 * необходимо изменить изображение на экране.
 */
void GLESScreen::exposeRegion(QRegion r, int windowIndex)
{
	QScreen::exposeRegion(r, windowIndex);
	if (gf_draw_begin(this->gfContext) != GF_ERR_OK)
	{
		qCritical("gf_draw_begin() failed");
		return;
	}

	if (gf_draw_flush(this->gfContext) != GF_ERR_OK)
	{
		qCritical("gf_draw_flush() failed");
	}

	gf_draw_end(this->gfContext);
}
