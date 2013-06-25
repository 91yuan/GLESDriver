#include <GLES/gl.h>
#include <cmath>
#include "helper.h"

GLuint createTexture(const QImage &img)
{
	if (img.isNull())
		return 0;

	int width = img.width();
	int height = img.height();
	int textureWidth;
	int textureHeight;
	GLuint texture;

	glGenTextures(1, &texture);
	textureWidth = nextPowerOfTwo(width);
	textureHeight = nextPowerOfTwo(height);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	switch (img.format())
	{
	case QImage::Format_RGB16:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0,
				GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB,
				GL_UNSIGNED_SHORT_5_6_5, img.bits());
		break;

	case QImage::Format_ARGB32_Premultiplied:
	case QImage::Format_ARGB32:
	case QImage::Format_RGB32:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
				GL_UNSIGNED_BYTE, img.bits());
		break;

	default:
		break;
	}

	return texture;
}


void drawQuad_helper(GLshort *coords, GLfloat *texCoords)
 {
     glEnableClientState(GL_TEXTURE_COORD_ARRAY);
     glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
     glEnableClientState(GL_VERTEX_ARRAY);
     glVertexPointer(2, GL_SHORT, 0, coords);
     glEnable(GL_TEXTURE_2D);
     glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
     glDisable(GL_TEXTURE_2D);
     glDisableClientState(GL_VERTEX_ARRAY);
     glDisableClientState(GL_TEXTURE_COORD_ARRAY);
 }


void drawQuad_helper(GLshort *coords, GLfloat *texCoords, int arraySize, int numArrays)
 {
     glEnable(GL_TEXTURE_2D);
     glEnableClientState(GL_TEXTURE_COORD_ARRAY);
     glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
     glEnableClientState(GL_VERTEX_ARRAY);
     glVertexPointer(2, GL_SHORT, 0, coords);

     for (int i = 0; i < numArrays-1; ++i)
         glDrawArrays(GL_TRIANGLE_STRIP, i*arraySize, arraySize);
     glDisableClientState(GL_VERTEX_ARRAY);
     glDisableClientState(GL_TEXTURE_COORD_ARRAY);
     glDisable(GL_TEXTURE_2D);
 }

void setRectCoords(GLshort *coords, QRect rect)
 {
     coords[0] = GLshort(rect.left());
     coords[1] = GLshort(rect.top());

     coords[2] = GLshort(rect.right());
     coords[3] = GLshort(rect.top());

     coords[4] = GLshort(rect.right());
     coords[5] = GLshort(rect.bottom());

     coords[6] = GLshort(rect.left());
     coords[7] = GLshort(rect.bottom());
 }



void setFlagCoords(GLshort *coords, QRectF screenGeometry, int frameNum, qreal progress)
{
    int coordIndex = 0;
    qreal waveHeight = 30.0*(1.0-progress);
    for (int j = 0; j < subdivisions-1; ++j) {
        for (int i = 0; i < subdivisions; ++i) {
            qreal c;
            c = screenGeometry.left()
                + (i * screenGeometry.width() / (subdivisions - 1))
                + waveHeight * qRound(sin((double)(M_PI * 20 * (double)(frameNum + i) / 180.0)))
                + waveHeight * qRound(cos((double)(M_PI * 20 * (double)(frameNum + j) / 180.0)));
            coords[coordIndex++] = qRound(c);
            c = screenGeometry.top()
                + (j * screenGeometry.height() / (subdivisions - 1))
                + waveHeight * sin((double)(M_PI * 20 * (double)(frameNum + i) / 180.0))
                + waveHeight * cos((double)(M_PI * 20 * (double)(frameNum + j) / 180.0));
            coords[coordIndex++] = qRound(c);
            c = screenGeometry.left() + (i * screenGeometry.width() / (subdivisions - 1))
                + waveHeight * sin((double)(M_PI * 20 * (double)(frameNum + i) / 180.0))
                + waveHeight * cos((double)(M_PI * 20 * (double)(frameNum + (j+1)) / 180.0));
            coords[coordIndex++] = qRound(c);

            c = screenGeometry.top()
                + ((j + 1) * screenGeometry.height() / (subdivisions - 1))
                + waveHeight * sin((double)(M_PI * 20 * (double)(frameNum + i) / 180.0))
                + waveHeight * cos((double)(M_PI * 20 * (double)(frameNum + (j + 1)) / 180.0));
            coords[coordIndex++] = qRound(c);
        }
    }
}

void setFlagTexCoords(GLfloat *texcoords, const QRectF &subTexGeometry,
                              const QRectF &textureGeometry,
                              int textureWidth, int textureHeight)
 {
     qreal topLeftX = (subTexGeometry.left() - textureGeometry.left())/textureWidth;
     qreal topLeftY = (textureGeometry.height() - (subTexGeometry.top() - textureGeometry.top()))/textureHeight;

     qreal width = (subTexGeometry.right() - textureGeometry.left())/textureWidth - topLeftX;
     qreal height = (textureGeometry.height() - (subTexGeometry.bottom() - textureGeometry.top()))/textureHeight - topLeftY;

     int coordIndex = 0;
     qreal spacing = subdivisions - 1;
     for (int j = 0; j < subdivisions-1; ++j) {
         for (int i = 0; i < subdivisions; ++i) {
             texcoords[coordIndex++] = topLeftX + (i*width) / spacing;
             texcoords[coordIndex++] = topLeftY + (j*height) / spacing;
             texcoords[coordIndex++] = topLeftX + (i*width) / spacing;
             texcoords[coordIndex++] = topLeftY + ((j+1)*height) / spacing;
         }
     }
 }
