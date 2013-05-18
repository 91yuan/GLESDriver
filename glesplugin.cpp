#include <QScreenDriverPlugin>
#include <QStringList>

#include "glesscreen.h"

class GLESScreenPlugin: public QScreenDriverPlugin
{
public:
	GLESScreenPlugin();

	QStringList keys() const;
	QScreen *create(const QString&, int displayId);
};

/*
 * �����������. ������ �� ������. �������� �����������
 * ������ QScreenDriverPlugin
 */
GLESScreenPlugin::GLESScreenPlugin() :
	QScreenDriverPlugin()
{
}

/*
 * ���������� ������ ����� QStringList ���������� ������ "glesdriver".
 * �� ��������� ����� ������ Qt ���������� ����� ��� ������� ����� ��������.
 * ��� ������� ���������� � ���� ��������� ���������� ����� ������� ���������:
 * ./app -qws -display glesdriver
 */
QStringList GLESScreenPlugin::keys() const
{
	return (QStringList() << "glesdriver");
}

/*
 * �������� �������� �� �������� driver.
 */
QScreen* GLESScreenPlugin::create(const QString& driver, int displayId)
{
	// ������ QScreenDriverPlugin �������� ���������������� � ��������, �������
	// ��������� driver � ������ �������
	if (driver.toLower() != "glesdriver")
		return 0;

	//������� ����� �������
	return new GLESScreen(displayId);
}

/*
 * ����� ������������ �������, ������� ������������ ������ GLESScreenPlugin
 * ��� glesdriver. ������������� ���� (TARGET) � ����� �������
 */
Q_EXPORT_PLUGIN2(glesdriver, GLESScreenPlugin)
