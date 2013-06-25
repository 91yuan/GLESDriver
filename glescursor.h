/*
 * Класс курсор.
 * */
#ifndef GLESCURSOR_H_
#define GLESCURSOR_H_

#include <QtGlobal>
#include <GLES/gl.h>

#include <QImage>
#include <QScreenCursor>

 class GLESCursor : public QScreenCursor
 {
 public:
     GLESCursor() : texture(0) {};
     ~GLESCursor();

     void set(const QImage &image, int hotx, int hoty);

     GLuint texture;
 };

#endif /* GLESCURSOR_H_ */
