#ifndef GLESSCREEN_H_
#define GLESSCREEN_H_

#include <QScreen>
#include <QtGlobal>
#include <QWSServer>
#include <QMap>
#include <QTimer>

#define GL_GLEXT_PROTOTYPES

#include <gf/gf.h>
#include <GLES/egl.h>
#include <GLES/gl.h>
#include <gf/gf3d.h>
#include <GLES/glext.h>


#include "glescursor.h"

const int frameSpan = 20;

class GLESCreenPrivate;

class GLESScreen: public QScreen
{
public:
	GLESScreen(int displayId);
	~GLESScreen();

	bool connect(const QString &displaySpec);
	bool initDevice();
	void shutdownDevice();
	void disconnect();

	void setMode(int, int, int){};
	void blank(bool){};

	void exposeRegion(QRegion r, int changing);

private:
    void redrawScreen();
	void setAttrs(int red, int green, int blue, int alpha, int depth);
	void drawWindow(QWSWindow *win);
	void drawQuad(const QRect &textureGeometry,
	                   const QRect &subGeometry,
	                   const QRect &screenGeometry);
    void drawQuadWavyFlag(const QRect &textureGeometry,
                          const QRect &subTexGeometry,
                          const QRect &screenGeometry,
                          qreal progress);
    void invalidateTexture(int windowIndex);

	GLESCursor *cursor;

	QTimer updateTimer;

	int displayId;
	gf_dev_t gfDev;
	gf_display_info_t displayInfo;
	gf_display_t gfDisplay;
	gf_layer_t gfLayer;
	gf_layer_info_t lInfo;
	gf_3d_target_t target;
	EGLDisplay eglDisplay;
	EGLSurface eglSurface;
	EGLContext eglContext;
	EGLConfig eglConfig;
	EGLint eglAttrs[13];

	GLESCreenPrivate *d_ptr;
	friend class GLESCreenPrivate;
};

#endif /* GLESSCREEN_H_ */

