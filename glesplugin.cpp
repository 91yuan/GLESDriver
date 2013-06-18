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
 * Конструктор. Ничего не делает. Вызывает конструктор
 * класса QScreenDriverPlugin
 */
GLESScreenPlugin::GLESScreenPlugin() :
	QScreenDriverPlugin()
{
}

/*
 * Возвращает список строк QStringList содержащий строку "glesdriver".
 * На основании этого списка Qt определяет ключи для запуска этого драйвера.
 * Для запуска приложения с этим драйвером необходимо будет указать следующее:
 * ./app -qws -display glesdriver
 */
QStringList GLESScreenPlugin::keys() const
{
	return (QStringList() << "glesdriver");
}

/*
 * Создание драйвера на оновании driver.
 */
QScreen* GLESScreenPlugin::create(const QString& driver, int displayId)
{
	// плагин QScreenDriverPlugin является нечувствительным к регистру, поэтому
	// переводим driver в нижний регистр
	if (driver.toLower() != "glesdriver")
		return 0;

	//создаем новый драйвер
	return new GLESScreen(displayId);
}

/*
 * вызов стандартного макроса, который экспортирует плагин GLESScreenPlugin
 * для glesdriver. Соответствует цели (TARGET) в файле проекта
 */
Q_EXPORT_PLUGIN2(glesdriver, GLESScreenPlugin)
