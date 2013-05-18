#ifndef GLESSCREEN_H_
#define GLESSCREEN_H_

#include <QScreen>
#include <QtGlobal>

#include <gf/gf.h>
#include <GLES/egl.h>
#include <GLES/gl.h>
#include <gf/gf3d.h>

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

private:
	void setAttrs(int red, int green, int blue, int alpha, int depth);

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
};

#endif /* GLESSCREEN_H_ */

