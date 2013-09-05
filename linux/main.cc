/*
* Copyright (C) 2012-2013 AirDC++ Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * In addition, as a special exception, compiling, linking, and/or
 * using OpenSSL with this program is allowed.
 */

#include <client/stdinc.h>
#include <client/DCPlusPlus.h>
#include <client/version.h>
#include <client/Util.h>
#include <client/TimerManager.h>
#include <client/Text.h>

#include "MainConsole.h"

#include <iostream>
#include <signal.h>

/*struct CommandlineArgs {
	std::vector<std::string> searchmagnets;
	std::vector<std::string> addmagnets;
	std::vector<std::string> urls;
	bool show;
	bool refresh;
	bool version;
	bool existing;
	CommandlineArgs() : show(FALSE), refresh(FALSE), version(FALSE), existing(FALSE) { }
};

void parseExtraArguments(int argc, char* argv[], CommandlineArgs* args)
{
	for (int i = 1; i < argc; i++)	// i = 1 since argv[0] is prgname
	{
		std::string arg(argv[i]);
		//if (WulforUtil::isHubURL(arg))
		//	args->urls.push_back(arg);
		//else if (WulforUtil::isMagnet(arg))
		//	args->addmagnets.push_back(arg);
	}
}*/

/*bool parseArguments(int *argc, char **argv[], CommandlineArgs* args)
{
	gchar **searchmagnet = NULL, **address = NULL, **addmagnet = NULL;
	gboolean show = FALSE, refresh = FALSE, version = FALSE, existing = FALSE;

	GOptionEntry entries[] = {
		{ "add-magnet", 'a', 0, G_OPTION_ARG_STRING_ARRAY, &addmagnet, N_("Attempt to add the magnet link directly to the download queue"), N_("URI") },
		{ "connect", 'c', 0, G_OPTION_ARG_STRING_ARRAY, &address, N_("Connect to the given hub"), N_("URI") },
		{ "existing", 'e', 0, G_OPTION_ARG_NONE, &existing, N_("Send commands to the existing instance (if applicable)"), NULL },
		{ "search-magnet", 'm', 0, G_OPTION_ARG_STRING_ARRAY, &searchmagnet, N_("Search for the given magnet link"), N_("URI") },
		{ "refresh", 'r', 0, G_OPTION_ARG_NONE, &refresh, N_("Initiate filelist refresh"), NULL },
		{ "show", 's', 0, G_OPTION_ARG_NONE, &show, N_("Show the running instance (default action)"), NULL },
		{ "version", 'v', 0, G_OPTION_ARG_NONE, &version, N_("Show version information and exit"), NULL },
		{ NULL }
	};

	GOptionContext *context;
	GError* error = NULL;
	context = g_option_context_new(N_("[URI...]"));
	g_option_context_add_main_entries(context, entries, NULL);
	// with gtk_get_option_group(TRUE) we don't need to call gtk_init with argc & argv in main.
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_set_summary(context, N_("File-sharing client for the Direct Connect network"));

	if (!g_option_context_parse(context, argc, argv, &error))
	{
		std::cerr << F_("Option parsing failed: %1%", % error->message) << std::endl;
		return FALSE;
	}
	else
	{
		if (show)
			args->show = TRUE;
		if (refresh)
			args->refresh = TRUE;
		if (version)
			args->version = TRUE;
		if (existing)
			args->existing = TRUE;
		if (searchmagnet)
		{
			int i = 0;
			for (gchar* it = searchmagnet[0]; it != NULL; it = searchmagnet[++i])
				args->searchmagnets.push_back(std::string(it));
			g_strfreev(searchmagnet);
		}
		if (addmagnet)
		{
			int i = 0;
			for (gchar* it = addmagnet[0]; it != NULL; it = addmagnet[++i])
				args->addmagnets.push_back(std::string(it));
			g_strfreev(addmagnet);
		}
		if (address)
		{
			int i = 0;
			for (gchar* it = address[0]; it != NULL; it = address[++i])
				args->urls.push_back(std::string(it));
			g_strfreev(address);
		}

		// Handle extra 'file' arguments passed to commandline
		parseExtraArguments(*argc, *argv, args);
	}
	g_option_context_free(context);

	return TRUE;
}*/

/*int handleArguments(const CommandlineArgs &args) 
{

	int retval = 1;
	if (args.refresh)
		retval = WulforUtil::writeIPCCommand("refresh");
	for (std::vector<std::string>::const_iterator it = args.searchmagnets.begin();
			retval > 0 && it != args.searchmagnets.end(); ++it)
	{
		retval = WulforUtil::writeIPCCommand(std::string("search-magnet ") + *it);
	}
	for (std::vector<std::string>::const_iterator it = args.addmagnets.begin();
			retval > 0 && it != args.addmagnets.end(); ++it)
	{
		retval = WulforUtil::writeIPCCommand(std::string("add-magnet ") + *it);
	}
	for (std::vector<std::string>::const_iterator it = args.urls.begin();
			retval > 0 && it != args.urls.end(); ++it)
	{
		retval = WulforUtil::writeIPCCommand(std::string("connect ") + *it);
	}

	if (retval > 0 && args.show) 
	{
		retval = WulforUtil::writeIPCCommand("show");
	}

	return retval;
}*/

int main(int argc, char *argv[])
{
	//CommandlineArgs args;

	/*std::string downloadPath;
	dcpp::Util::PathsMap pathsMap;
	pathsMap[dcpp::Util::PATH_RESOURCES] = _DATADIR;
	pathsMap[dcpp::Util::PATH_LOCALE] = _DATADIR "/locale";
	if (g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD) != NULL)
	{
		downloadPath = g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD);
		pathsMap[dcpp::Util::PATH_DOWNLOADS] = downloadPath + PATH_SEPARATOR_STR;
	}*/
	dcpp::Util::initialize();

	//IntlUtil::initialize();

	//if (!parseArguments(&argc, &argv, &args))
	//{
	//	return -1;
	//}

	/*if (args.version)
	{
		printVersionInfo();
		return 0;
	}

	// Check if profile is locked
	if (WulforUtil::profileIsLocked())
	{
		int retval = handleArguments(args);

		if (retval < 0) 
		{
			std::cerr << F_("Failed to communicate with existing instance: %1%", % dcpp::Util::translateError(retval)) << std::endl;
			return -1;
		}

		if (retval == 1 && WulforUtil::writeIPCCommand("show") <= 0)	
		{
			// Show profile lock error only if profile is locked and talking to running instance
			// fails for some reason
			std::string message = _("Unable to communicate with existing instance and only one instance is allowed per profile.");

			GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message.c_str());
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);

			return -1;
		}

		return 0;
	}

	if (args.existing) // If we're still here and --existing was given, bail out since no running instance was found
	{
		std::cerr << _("No running instance found") << std::endl;
		return 0;
	}*/

	// Start the DC++ client core
	dcpp::startup(nullptr, 
		[&](const std::string& str, bool isQuestion, bool isError) {
			if (isQuestion) {
				std::cout << dcpp::Text::fromUtf8(str) << std::endl;
				return false;
			} else {
				std::cout << dcpp::Text::fromUtf8(str) << std::endl;
			}

			return true;
	}, nullptr, nullptr);

	dcpp::TimerManager::getInstance()->start();

	//g_thread_init(NULL);
	//gdk_threads_init();
	//glade_init();
	//g_set_application_name("AirDC++");

	signal(SIGPIPE, SIG_IGN);
	unique_ptr<MainConsole> console(new MainConsole);

	console->run();

	console.reset();

	/*WulforSettingsManager::newInstance();
	WulforManager::start();
	handleArguments(args);
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
	WulforManager::stop();
	WulforSettingsManager::deleteInstance();*/

	dcpp::shutdown(nullptr, nullptr);

	return 0;
}

