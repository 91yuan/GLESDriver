#include "glesscreen.h"
#include "helper.h"
#include <QBrush>

// Структура WindowInfo. Хранит информацию об окне - id текстуры этого окна
 struct WindowInfo
 {
     WindowInfo() : texture(0) {}
     GLuint texture;
 };

 // Карта ставящая в соответствие окну информацию о нем
 static QMap<QWSWindow*, WindowInfo*> windowMap;


class GLESCreenPrivate : public QObject
{
	Q_OBJECT

	public:
		GLESCreenPrivate(GLESScreen *screen) {this->screen = screen;};

	public slots:
	     void windowEvent(QWSWindow *w, QWSServer::WindowEvent e);
	     void redrawScreen();

	private:
	     GLESScreen *screen;
};

// Метод обрабатывающие событие таймера по перерисовке экрана
void GLESCreenPrivate::redrawScreen()
{
    screen->updateTimer.stop();		// Останавливает таймер
    screen->redrawScreen();	// Вызывает перерисовку экрана
}

// Метод обрабатывающие событие окна.
void GLESCreenPrivate::windowEvent(QWSWindow *window, QWSServer::WindowEvent event)
{
	switch (event)
	{
	case QWSServer::Create:		// Если было создано новое окно, то создает объект
		windowMap[window] = new WindowInfo;		// WindowInfo и записывает его в карту
		break;
	case QWSServer::Destroy:	// Если было уничтожено окно, то удаляет его из карты
		delete windowMap[window];
		windowMap.remove(window);
		break;
	default:
		break;
	}
}



GLESScreen::GLESScreen(int displayId)
	:QScreen(displayId)
{
	this->d_ptr = new GLESCreenPrivate(this);

	this->cursor = 0;

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

	// Событие таймера подключаем к функции redrawScreen()
	this->d_ptr->connect(&updateTimer, SIGNAL(timeout()), d_ptr, SLOT(redrawScreen()));

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

	// Подключение к слотам событий
	// Событие окна, подключаем к функции windowEvent()
    d_ptr->connect(QWSServer::instance(),
                   SIGNAL(windowEvent(QWSWindow*, QWSServer::WindowEvent)),
                   SLOT(windowEvent(QWSWindow*, QWSServer::WindowEvent)));

	// Инициализация программного курсора
	//QScreenCursor::initSoftwareCursor();
    this->cursor = new GLESCursor();
	qt_screencursor = this->cursor;

	return true;
}

// Освобождение всех выделенных ресурсов
void GLESScreen::shutdownDevice()
{
    delete this->cursor;
    this->cursor = 0;
    qt_screencursor = 0;

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
    if ((r & region()).isEmpty())
        return;

    invalidateTexture(windowIndex);

    if (!this->updateTimer.isActive())
        this->updateTimer.start(frameSpan);
}


// Метод отрисовки квадрата
void GLESScreen::drawQuad(const QRect &textureGeometry,
                             const QRect &subGeometry,
                             const QRect &screenGeometry)
 {
     qreal textureWidth = qreal(nextPowerOfTwo(textureGeometry.width()));
     qreal textureHeight = qreal(nextPowerOfTwo(textureGeometry.height()));

     GLshort coords[8];
     setRectCoords(coords, screenGeometry);

     GLfloat texcoords[8];
     texcoords[0] = (subGeometry.left() - textureGeometry.left()) / textureWidth;
     texcoords[1] = (subGeometry.top() - textureGeometry.top()) / textureHeight;

     texcoords[2] = (subGeometry.right() - textureGeometry.left()) / textureWidth;
     texcoords[3] = (subGeometry.top() - textureGeometry.top()) / textureHeight;

     texcoords[4] = (subGeometry.right() - textureGeometry.left()) / textureWidth;
     texcoords[5] = (subGeometry.bottom() - textureGeometry.top()) / textureHeight;

     texcoords[6] = (subGeometry.left() - textureGeometry.left()) / textureWidth;
     texcoords[7] = (subGeometry.bottom() - textureGeometry.top()) / textureHeight;

     drawQuad_helper(coords, texcoords);
 }


void GLESScreen::drawQuadWavyFlag(const QRect &textureGeometry,
                                    const QRect &subTexGeometry,
                                    const QRect &screenGeometry,
                                    qreal progress)
{
    const int textureWidth = nextPowerOfTwo(textureGeometry.width());
    const int textureHeight = nextPowerOfTwo(textureGeometry.height());

    static int frameNum = 0;

    GLshort coords[subdivisions*subdivisions*2*2];
    setFlagCoords(coords, screenGeometry, frameNum++, progress);

    GLfloat texcoords[subdivisions*subdivisions*2*2];
    setFlagTexCoords(texcoords, subTexGeometry, textureGeometry,
                     textureWidth, textureHeight);

    drawQuad_helper(coords, texcoords, subdivisions*2, subdivisions);
}


// Метод уничтожает текстуру для указанного окна
void GLESScreen::invalidateTexture(int windowIndex)
{
    if (windowIndex < 0)
        return;

    QList<QWSWindow*> windows = QWSServer::instance()->clientWindows();
    if (windowIndex > windows.size() - 1)
        return;

    QWSWindow *win = windows.at(windowIndex);
    if (!win)
        return;

    WindowInfo *info = windowMap[win];
    if (info->texture) {
        glDeleteTextures(1, &info->texture);
        info->texture = 0;
    }
}

/*
 * Рисует окно указанное в аргументе win.
 */
void GLESScreen::drawWindow(QWSWindow *win)
 {
     const QRect screenRect = win->allocatedRegion().boundingRect();
     QRect drawRect = screenRect;

     glColor4f(1.0, 1.0, 1.0, 1.0);
     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
     glEnable(GL_BLEND);

     QWSWindowSurface *surface = win->windowSurface();
     if (!surface)
         return;

     drawQuad(win->requestedRegion().boundingRect(), screenRect, drawRect);
 }

/*
 * Обновляет изображение на экране. Компанует все окна вместе. Выводит изображение
 * на экран.
 */
 void GLESScreen::redrawScreen()
 {
	 glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
     glMatrixMode(GL_PROJECTION);
     glPushMatrix();
     glMatrixMode(GL_MODELVIEW);
     glPushMatrix();

     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     glOrthof(0, w, h, 0, -999999, 999999);
     glViewport(0, 0, w, h);
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();

     // Заполняет задний фон
     QColor bgColor = QWSServer::instance()->backgroundBrush().color();
     glClearColor(bgColor.redF(), bgColor.greenF(),
                  bgColor.blueF(), bgColor.alphaF());
     glClear(GL_COLOR_BUFFER_BIT);

     // Рисуте все окна
     glDisable(GL_CULL_FACE);
     glDisable(GL_DEPTH_TEST);
     glDisable(GL_STENCIL_TEST);
     glEnable(GL_BLEND);
     glBlendFunc(GL_ONE, GL_ZERO);
     glDisable(GL_ALPHA_TEST);
     glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

     QList<QWSWindow*> windows = QWSServer::instance()->clientWindows();
     for (int i = windows.size() - 1; i >= 0; --i) {
         QWSWindow *win = windows.at(i);
         QWSWindowSurface *surface = win->windowSurface();
         if (!surface)
             continue;

         WindowInfo *info = windowMap[win];

         if (!info->texture) {
             info->texture = createTexture(surface->image());
         }
         glBindTexture(GL_TEXTURE_2D, info->texture);
         drawWindow(win);
     }

     // Рисует курсор поверх всех окон
     const GLESCursor *cursor = this->cursor;
     if (cursor->texture) {
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         glBindTexture(GL_TEXTURE_2D, this->cursor->texture);
         drawQuad(cursor->boundingRect(), cursor->boundingRect(),
                  cursor->boundingRect());
     }

     glPopMatrix();
     glMatrixMode(GL_PROJECTION);
     glPopMatrix();
     glMatrixMode(GL_MODELVIEW);

     eglSwapBuffers(this->eglDisplay, this->eglSurface);
 }


