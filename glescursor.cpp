/*
 * Реализация класса GLESCursor
 */
#include "helper.h"

// консруктор
GLESCursor::~GLESCursor()
 {
     if (texture)
         glDeleteTextures(1, &texture);
 }


// Метод set устанавливает картинку курсора и его координаты
 void GLESCursor::set(const QImage &image, int hotx, int hoty)
 {
     if (texture)
         glDeleteTextures(1, &texture);

     if (image.isNull())
         texture = 0;
     else
         texture = createTexture(image.convertToFormat(QImage::Format_ARGB32));

     QScreenCursor::set(image, hotx, hoty);
 }
