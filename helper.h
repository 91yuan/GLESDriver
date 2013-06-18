/*
 * Вспомогательные функции.
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <QtGlobal>
#include <QImage>

// Создает текстуру из переданной картинки и возвращает id этой текстуры
GLuint createTexture(const QImage &img);

// Вспомогательные функции, для функци рисования квадратов
void drawQuad_helper(GLshort *coords, GLfloat *texCoords);
void drawQuad_helper(GLshort *coords, GLfloat *texCoords, int arraySize, int numArrays);
void setRectCoords(GLshort *coords, QRect rect);
void setFlagCoords(GLshort *coords, QRectF screenGeometry, int frameNum, qreal progress);
void setFlagTexCoords(GLfloat *texcoords, const QRectF &subTexGeometry,
                              const QRectF &textureGeometry,
                              int textureWidth, int textureHeight);

// Количество ячеек сетки используемое при тесселяции
const int subdivisions = 10;


// Округляет число до ближайшей большей степени двойки
inline static uint nextPowerOfTwo(uint v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

#endif /* HELPER_H_ */
