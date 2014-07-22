
/* Copyright (c) 2013-2014, ahmet.bilgili@epfl.ch
 *
 * This file is part of Lunchbox <https://github.com/Eyescale/Lunchbox>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <lunchbox/uri.h>

#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <exception>

namespace lunchbox
{

namespace detail
{

struct URIData
{
    URIData() : port( 0 ) {}

    std::string scheme;
    std::string userinfo;
    std::string host;
    uint16_t port;
    std::string path;
    std::string query;
    std::string fragment;
};

class uri_parse : public std::exception
{
public:
    uri_parse( const std::string& uri )
    {
        _error << "Error parsing URI string: " << uri << std::endl;
    }

    uri_parse( const uri_parse& excep )
    {
        _error << excep._error;
    }

    virtual ~uri_parse() throw() {}

    virtual const char* what() const throw() { return _error.str().c_str(); }

private:
    std::stringstream _error;
};

class URI
{
public:
    URI( const std::string &uri )
       : _uri( uri )
    {
        boost::regex expr(
            "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?",
            boost::regex::perl|boost::regex::icase );

        boost::match_results< std::string::const_iterator > results;
        if( !boost::regex_search( uri, results, expr ) )
            throw uri_parse( _uri );

        const std::string& schema = std::string( results[2].first, results[2].second );
        if( schema.empty() )
            throw uri_parse( _uri );

        _uriData.scheme = schema;
        const std::string& userHost = std::string( results[4].first, results[4].second );

        if( !userHost.empty() )
        {
            std::vector< std::string > splitUserHost;
            std::string hostPort;
            boost::algorithm::split( splitUserHost, userHost, boost::is_any_of("@") );
            if( splitUserHost.size() == 2 ) // for ex: user:pass@hello.com:port
            {
                _uriData.userinfo = splitUserHost[ 0 ];
                hostPort = splitUserHost[ 1 ];
            }
            else
                hostPort = splitUserHost[ 0 ];

            std::vector< std::string > splitHostPort;
            boost::algorithm::split( splitHostPort, hostPort, boost::is_any_of(":") );
            _uriData.host = splitHostPort[ 0 ];

            if( splitUserHost.size() == 2 ) // for ex: user:pass@hello.com:port
                _uriData.port = boost::lexical_cast< uint16_t > ( splitHostPort[ 1 ] );
        }

        _uriData.path = std::string( results[5].first, results[5].second );
        _uriData.query = std::string( results[7].first, results[7].second );
        _uriData.fragment = std::string( results[9].first, results[9].second );
    }

    const URIData& getData() const { return _uriData; }
    const std::string& getURI() const { return _uri; }

private:

     std::string _uri;
     URIData _uriData;
};

}

URI::URI( const std::string &uri )
   : _impl( new detail::URI( uri ) )
{
}

lunchbox::URI::~URI()
{
    delete _impl;
}

const std::string &URI::getScheme() const
{
    return _impl->getData().scheme;
}

const std::string &URI::getHost() const
{
    return _impl->getData().host;
}

uint16_t URI::getPort() const
{
    return _impl->getData().port;
}

const std::string &URI::getUserinfo() const
{
    return _impl->getData().userinfo;
}

const std::string& URI::getPath() const
{
    return _impl->getData().path;
}

const std::string& URI::getQuery() const
{
    return _impl->getData().query;
}

const std::string &URI::getFragment() const
{
    return _impl->getData().fragment;
}



}
