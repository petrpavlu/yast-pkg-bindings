/* ------------------------------------------------------------------------------
 * Copyright (c) 2008 Novell, Inc. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may find
 * current contact information at www.novell.com.
 * ------------------------------------------------------------------------------
 */

/*
   File:	$Id$
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Functions for accessing services in the package management
   Namespace:   Pkg
*/

#include "PkgFunctions.h"
#include "log.h"

#include <ycp/YCPValue.h>
#include <ycp/YCPString.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPList.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPVoid.h>


/**
   @builtin ServiceAliases
   @short Returns aliases of all known services.
   @return list<string> list of known aliases
*/
YCPValue PkgFunctions::ServiceAliases()
{
    YCPList ret;

    ServiceManager::Services services = service_manager.GetServices();

    for_(it, services.begin(), services.end())
    {
	ret->add(YCPString(it->alias()));
    }

    return ret;
}

/**
   @builtin ServiceAdd
   @short Add a new service
   @param alias alias of the new service, must be unique
   @param url URL of the serivce
   @return boolean false if failed
*/
YCPValue PkgFunctions::ServiceAdd(const YCPString &alias, const YCPString &url)
{
    try
    {
	if (alias.isNull() || url.isNull())
	{
	    y2error("Found nil parameter in Pkg::ServiceAdd()");
	    return YCPBoolean(false);
	}

	return YCPBoolean(service_manager.AddService(alias->value(), url->value()));
    }
    catch(...)
    {
    }

    return YCPBoolean(false);
}

/**
   @builtin ServiceDelete
   @short Remove a service
   @param alias alias of the service to remove
   @return boolean false if failed
*/
YCPValue PkgFunctions::ServiceDelete(const YCPString &alias)
{
    try
    {
	if (alias.isNull())
	{
	    y2error("Found nil parameter in Pkg::ServiceDelete()");
	    return YCPBoolean(false);
	}

	return YCPBoolean(service_manager.RemoveService(alias->value()));
    }
    catch(...)
    {
    }

    return YCPBoolean(false);
}


/**
   @builtin ServiceGet
   @short Get detailed properties of a service
   @param alias alias of the service
   @return map Service data in map $[ "alias":string, "name":string, "url":string, "autorefresh":boolean, "enabled":boolean, "file":string ]. "file" is name of the file from which the service has been read, it is empty if the service has not been saved yet
*/
YCPValue PkgFunctions::ServiceGet(const YCPString &alias)
{
    if (alias.isNull())
    {
	y2error("Error: nil service name");
	return YCPVoid();
    }

    YCPMap ret;
    zypp::ServiceInfo s(service_manager.GetService(alias->value()));

    ret->add(YCPString("alias"), YCPString(s.alias()));
    ret->add(YCPString("name"), YCPString(s.name()));
    ret->add(YCPString("url"), YCPString(s.url().asString()));
    ret->add(YCPString("autorefresh"), YCPBoolean(s.autorefresh()));
    ret->add(YCPString("enabled"), YCPBoolean(s.enabled()));
    ret->add(YCPString("file"), YCPString(s.filepath().asString()));

    return ret;
}

/******************************************************************************
 * @builtin SourceURL
 *
 * @short Get full service URL (including password!)
 * @param alias alias of the service
 * @return string URL or empty string on failure
 **/
YCPValue
PkgFunctions::ServiceURL(const YCPString &alias)
{
    if (alias.isNull())
    {
	y2error("Error: nil service name");
	return YCPString("");
    }

    zypp::ServiceInfo s(service_manager.GetService(alias->value()));

    return YCPString(s.url().asCompleteString());
}

/**
   @builtin ServiceSet
   @short Set properties of a service
   @param alias old of the service, if the service is being renamed here is the old alias, the new alias is in the map (the second parameter)
   @param service new properties of the service, see Pkg::ServiceGet() for the description
   @return boolean false if failed
*/
YCPValue PkgFunctions::ServiceSet(const YCPString &old_alias, const YCPMap &service)
{
    if (old_alias.isNull() || service.isNull())
    {
	y2error("Error: nil parameter");
	return YCPBoolean(false);
    }

    try
    {
	zypp::ServiceInfo s(service_manager.GetService(old_alias->value()));
	YCPValue v;

	v = service->value(YCPString("alias"));
	if(!v.isNull() && v->isString())
	{
	    s.setAlias(v->asString()->value());
	}

	v = service->value(YCPString("name"));
	if(!v.isNull() && v->isString())
	{
	    s.setName(v->asString()->value());
	}

	v = service->value(YCPString("url"));
	if(!v.isNull() && v->isString())
	{
	    s.setUrl(v->asString()->value());
	}

	v = service->value(YCPString("autorefresh"));
	if(!v.isNull() && v->isBoolean())
	{
	    s.setAutorefresh(v->asBoolean()->value());
	}

	v = service->value(YCPString("enabled"));
	if(!v.isNull() && v->isBoolean())
	{
	    s.setEnabled(v->asBoolean()->value());
	}

	// note: do not allow to change the file name?

	return YCPBoolean(service_manager.SetService(old_alias->value(), s));
    }
    catch(...)
    {
    }

    return YCPBoolean(false);
}

/**
   @builtin ServicesSave
   @short Save the current service configuration to disk.
   @return boolean false if failed
*/
YCPValue PkgFunctions::ServicesSave()
{
    try
    {
	zypp::RepoManager repomanager = CreateRepoManager();
	return YCPBoolean(service_manager.SaveServices(repomanager));
    }
    catch(...)
    {
    }

    return YCPBoolean(false);
}

/**
   @builtin ServicesLoad
   @short Load service configurations from disk. If the services have alrady been loaded it does nothing.
   @return boolean false if failed
*/
YCPValue PkgFunctions::ServicesLoad()
{
    try
    {
	zypp::RepoManager repomanager = CreateRepoManager();
	return YCPBoolean(service_manager.LoadServices(repomanager));
    }
    catch(...)
    {
    }

    return YCPBoolean(false);
}

/**
   @builtin ServiceRefresh
   @short Refresh the service, the service must already be saved on the system!
   @param alias alias of the service to refresh
   @return boolean false if failed
*/
YCPValue PkgFunctions::ServiceRefresh(const YCPString &alias)
{
    try
    {
	if (alias.isNull())
	{
	    y2error("Error: nil parameter");
	    return YCPBoolean(false);
	}

	zypp::RepoManager repomanager = CreateRepoManager();
	return YCPBoolean(service_manager.RefreshService(alias->value(), repomanager));
    }
    catch(...)
    {
    }

    return YCPBoolean(false);
}

/**
   @builtin ServicesReset
   @short Reset the list of known services. All changes are lost.
*/
YCPValue PkgFunctions::ServicesReset()
{
    service_manager.Reset();
    return YCPVoid();
}

/**
   @builtin ServiceProbe
   @short Probe service type at a URL
   @param url URL of the service
   @return string probed type or "NONE" if there is no service
*/
YCPValue PkgFunctions::ServiceProbe(const YCPString &url)
{
    if (url.isNull())
    {
	y2error("URL is nil");
	return YCPVoid();
    }

    try
    {
	const zypp::RepoManager repomanager = CreateRepoManager();
	return YCPString(service_manager.Probe(zypp::Url(url->asString()->value()), repomanager));
    }
    catch(...)
    {
	return YCPVoid();
    }
}