#include "recordinformationtool.h"

RecordInformationTool* RecordInformationTool::p_Instance = nullptr;
RecordInformationTool::RecordInformationTool(QObject *parent) : QObject(parent)
{
	m_DICOMVisible  = true;
	m_WindowVisible = false;
	m_LayoutChange  = false;
	m_WindowLayoutIndex = 3;

	m_OpenFilePathList.clear();
}

RecordInformationTool* RecordInformationTool::getInstance()
{
	if (nullptr == p_Instance)
	{
		p_Instance = new RecordInformationTool;
	}
	return p_Instance;
}

void RecordInformationTool::setPopWidgetDICOMVisble(bool state)
{
	m_DICOMVisible = state;
}

bool RecordInformationTool::getPopWidgetDICOMVisble()
{
	return m_DICOMVisible;
}

void RecordInformationTool::setWindowVisible(bool state)
{
	m_WindowVisible = state;
}

bool RecordInformationTool::getWindowVisible()
{
	return m_WindowVisible;
}

void RecordInformationTool::setWindowLayout(int index)
{
	m_WindowLayoutIndex = index;
}

int RecordInformationTool::getWindowLayout() const
{
	return m_WindowLayoutIndex;
}

void RecordInformationTool::setWindowLayoutChange(bool state)
{
	m_LayoutChange = state;
}

bool RecordInformationTool::getWindowLayoutChange()
{
	return m_LayoutChange;
}

void RecordInformationTool::setOpenFilePathList(const QStringList& list)
{
	m_OpenFilePathList = list;
}

QStringList RecordInformationTool::getOpenFilePathList()
{
	return m_OpenFilePathList;
}

void RecordInformationTool::clearFilesList()
{
	m_OpenFilePathList.clear();
}
