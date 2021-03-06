// index_set.cpp

/**
*    Copyright (C) 2013 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mongo/bson/util/builder.h"
#include "mongo/db/index_set.h"

namespace mongo {

    void IndexPathSet::addPath( const StringData& path ) {
        string s;
        if ( getCanonicalIndexField( path, &s ) ) {
            _canonical.insert( s );
        }
        else {
            _canonical.insert( path.toString() );
        }
    }

    void IndexPathSet::clear() {
        _canonical.clear();
    }

    bool IndexPathSet::mightBeIndexed( const StringData& path ) const {
        StringData use = path;
        string x;
        if ( getCanonicalIndexField( path, &x ) )
            use = StringData( x );

        for ( std::set<string>::const_iterator i = _canonical.begin();
              i != _canonical.end();
              ++i ) {

            StringData idx( *i );

            if ( _startsWith( use, idx ) )
                return true;

            if ( _startsWith( idx, use ) )
                return true;
        }

        return false;
    }

    bool IndexPathSet::_startsWith( const StringData& a, const StringData& b ) const {
        if ( !a.startsWith( b ) )
            return false;

        // make sure there is a dot or EOL right after

        if ( a.size() == b.size() )
            return true;

        return a[b.size()] == '.';
    }

    bool getCanonicalIndexField( const StringData& fullName, string* out ) {
        // check if fieldName contains ".$" or ".###" substrings (#=digit) and skip them
        // however do not skip the first field even if it meets these criteria

        if ( fullName.find( '.' ) == string::npos )
            return false;

        bool modified = false;

        StringBuilder buf;
        for ( size_t i=0; i<fullName.size(); i++ ) {

            char c = fullName[i];

            if ( c != '.' ) {
                buf << c;
                continue;
            }

            if ( i + 1 == fullName.size() ) {
                // ends with '.'
                buf << c;
                continue;
            }

            // check for ".$", skip if present
            if ( fullName[i+1] == '$' ) {
                // only do this if its not something like $a
                if ( i + 2 >= fullName.size() || fullName[i+2] == '.' ) {
                    i++;
                    modified = true;
                    continue;
                }
            }

            // check for ".###" for any number of digits (no letters)
            if ( isdigit( fullName[i+1] ) ) {
                size_t j = i;
                // skip digits
                while ( j+1 < fullName.size() && isdigit( fullName[j+1] ) )
                    j++;

                if ( j+1 == fullName.size() || fullName[j+1] == '.' ) {
                    // only digits found, skip forward
                    i = j;
                    modified = true;
                    continue;
                }
            }

            buf << c;
        }

        if ( !modified )
            return false;

        *out = buf.str();
        return true;
    }

} // namespace mongo
