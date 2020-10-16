#include "Application.h"
#include "Nodable.h" 	// for NODABLE_VERSION
#include "Log.h" 		// for LOG_DEBUG
#include "Parser.h"
#include "ContainerView.h"
#include "ApplicationView.h"
#include "Variable.h"
#include "DataAccess.h"
#include "FileView.h"

#include "File.h"
#include <iostream>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <string>
#include <algorithm>
#include <future>

using namespace Nodable;

Application::Application(const char* _name):currentFileIndex(0)
{
	set("__class__", "Application");
	setLabel(_name);
	addComponent(new ApplicationView(_name, this));
}

Application::~Application()
{
	for (auto it = loadedFiles.begin(); it != loadedFiles.end(); it++)
		delete* it;
}

bool Application::init()
{
	auto view = getComponent<ApplicationView>();
	view->init();

	return true;
}

UpdateResult Application::update()
{
	auto file = getCurrentFile();
	
	if (!file)
	{
		return UpdateResult::Failed;
	}
	else
	{
		const auto fileUpdateResult = file->update();

		if( quit )
        {
		    return UpdateResult::Stopped;
        }
		else
        {
            return fileUpdateResult;
        }
	}
}

void Application::stopExecution()
{
	quit = true;
}

void Application::shutdown()
{
}

bool Application::openFile(std::filesystem::path _filePath)
{		
	auto file = File::CreateFileWithPath(_filePath);

	if (file != nullptr)
	{
		loadedFiles.push_back(file);
		setCurrentFileWithIndex(loadedFiles.size() - 1);
	}

	return file != nullptr;
}

void Application::saveCurrentFile() const
{
	auto currentFile = loadedFiles.at(currentFileIndex);
	if (currentFile)
		currentFile->save();
}

void Application::closeCurrentFile()
{
	auto currentFile = loadedFiles.at(currentFileIndex);
	if (currentFile != nullptr)
	{
		auto it = std::find(loadedFiles.begin(), loadedFiles.end(), currentFile);
		loadedFiles.erase(it);
		delete currentFile;
		if (currentFileIndex > 0)
			setCurrentFileWithIndex(currentFileIndex - 1);
		else
			setCurrentFileWithIndex(currentFileIndex);
	}
}

File* Application::getCurrentFile()const {

	if (loadedFiles.size() > currentFileIndex) {
		return loadedFiles.at(currentFileIndex);
	}
	return nullptr;
}

void Application::setCurrentFileWithIndex(size_t _index)
{
	if (loadedFiles.size() > _index)
	{
		auto newFile = loadedFiles.at(_index);
		currentFileIndex = _index;
	}
}

void Application::SaveNode(Node* _node)
{
    auto component = new DataAccess;
	_node->addComponent(component);
	component->update();
	_node->deleteComponent<DataAccess>();
}

std::filesystem::path Application::getAssetPath(const char* _fileName)const
{
	auto assetPath = this->assetsFolderPath;
	assetPath /= _fileName;
	return assetPath;
}

std::future<bool> Application::openURL(std::string _URL) const {

    auto result = std::async(std::launch::async, [&_URL]
    {
#ifdef WIN32
        std::string command("start");
#else
        std::string command("x-www-browser");
#endif

        std::string op = command + " " + _URL;
        auto success = system(op.c_str()) == 0;

        if (!success)
        {
            LOG_ERROR( 0u, "Unable to open %s. Because the command %s is not available on your system.",
                       _URL.c_str(), command.c_str());
        }

        return success;
    });

    return result;
}
