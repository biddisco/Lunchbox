
/* Copyright (c) 2014-2015, Stefan.Eilemann@epfl.ch
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

#define TEST_RUNTIME 500 //seconds
#include <test.h>
#include <lunchbox/clock.h>
#include <lunchbox/os.h>
#include <lunchbox/persistentMap.h>
#ifdef LUNCHBOX_USE_LEVELDB
#  include <leveldb/db.h>
#endif
#ifdef LUNCHBOX_USE_SKV
#  include <FxLogger/FxLogger.hpp>
#endif
#include <boost/format.hpp>
#include <stdexcept>
#include <deque>

using lunchbox::PersistentMap;

const int ints[] = { 17, 53, 42, 65535, 32768 };
const size_t numInts = sizeof( ints ) / sizeof( int );
const int64_t loopTime = 2000;

// for benchmark scripting
int arg1_queuedepth = 1;
int arg2_nservers   = 1;
bool useargs = false;

template< class T > void insertVector( PersistentMap& map )
{
    std::vector< T > vector;
    for( size_t i = 0; i < numInts; ++i )
        vector.push_back( T( ints[ i ] ));

    TEST( map.insert( typeid( vector ).name(), vector ));
}

template< class T > void readVector( const PersistentMap& map )
{
    const std::vector< T >& vector =
        map.getVector< T >( typeid( vector ).name( ));
    TESTINFO( vector.size() ==  numInts, vector.size() << " != " << numInts );
    for( size_t i = 0; i < numInts; ++i )
        TEST( vector[ i ] == T( ints[i] ));
}

void read( const PersistentMap& map )
{
    const std::set< uint32_t >& bigSet =
        map.getSet< uint32_t >( "std::set< uint32_t >" );
    TEST( bigSet.size() == 1000 );
    for( uint32_t i = 1; i <= 1000; ++i )
        TEST( bigSet.find( i ) != bigSet.end( ));

    TEST( map[ "foo" ] == "bar" );
    TEST( map[ "bar" ].empty( ));
    TEST( map.get< bool >( "bValue" ) == true );
    TEST( map.get< int >( "iValue" ) == 42 );

    readVector< int >( map );
    readVector< uint16_t >( map );

    const std::set< int >& set = map.getSet< int >( "std::set< int >" );
    TESTINFO( set.size() ==  numInts, set.size() << " != " << numInts );
    for( size_t i = 0; i < numInts; ++i )
        TESTINFO( set.find( ints[i] ) != set.end(),
                  ints[i] << " not found in set" );
}

void read( const std::string& uri )
{
    PersistentMap map( uri );
    read( map );
}

void setup( const std::string& uri )
{
    PersistentMap map( uri );
    TEST( map.insert( "foo", "bar" ));
    TEST( map.contains( "foo" ));
    TESTINFO( map[ "foo" ] == "bar",
              map[ "foo" ] << " length " << map[ "foo" ].length( ));
    TEST( map[ "bar" ].empty( ));

    TEST( map.insert( "the quick brown fox", "jumped over something" ));
    TESTINFO( map[ "the quick brown fox" ] == "jumped over something",
              map[ "the quick brown fox" ] );

    TEST( map.insert( "hans", std::string( "dampf" )));
    TESTINFO( map[ "hans" ] == "dampf", map[ "hans" ] );

    const bool bValue = true;
    TEST( map.insert( "bValue", bValue ));
    TEST( map.get< bool >( "bValue" ) == bValue );

    const int iValue = 42;
    TEST( map.insert( "iValue", iValue ));
    TEST( map.get< int >( "iValue" ) == iValue );

    insertVector< int >( map );
    insertVector< uint16_t >( map );
    readVector< int >( map );
    readVector< uint16_t >( map );

    std::set< int > set( ints, ints + numInts );
    TEST( map.insert( "std::set< int >", set ));

    std::set< uint32_t > bigSet;
    for( uint32_t i = 1; i <= 1000; ++i )
        bigSet.insert( i );
    TEST( map.insert( "std::set< uint32_t >", bigSet ));

    read( map );
}

// value length is 8*value_qwords
void benchmark( const std::string& uri, const uint64_t queueDepth, const int value_qwords )
{
    PersistentMap map( uri );
    map.setQueueDepth( queueDepth );

    // Prepare keys
    lunchbox::Strings keys;
    keys.resize( queueDepth + 1 );
    for( uint64_t i = 0; i <= queueDepth; ++i )
        keys[i].assign( reinterpret_cast< char* >( &i ), 8 );

    const size_t value_length = 8*value_qwords;
    float value_bytes_transfer = 0;

    std::string value;
    value.reserve(value_length);

    // write performance
    lunchbox::Clock clock;
    uint64_t i = 0;
    while( clock.getTime64() < loopTime )
    {
        std::string& key = keys[ i % (queueDepth+1) ];
        *reinterpret_cast< uint64_t* >( &key[0] ) = i;
        // repeat the key N times as the value
        for (int q=0; q<value_qwords; ++q) *reinterpret_cast< uint64_t* >( &value[q*8] ) = i;
        map.insert( key, &value[0], value_length );
        ++i;
    }
    map.flush();
    const float insertTime = clock.getTimef();
    const uint64_t wOps = i;
    value_bytes_transfer = i*value_length;
    TEST( i > queueDepth );
    // NB time is in milliseconds, so use 1000 factor
    float write_value_BW_MBs = 1000.0*value_bytes_transfer/(insertTime*1024.0*1024.0);

    // read performance
    std::string key;
    key.assign( reinterpret_cast< char* >( &i ), 8 );

    clock.reset();
    if( queueDepth == 0 ) // sync read
    {
        for( i = 0; i < wOps && clock.getTime64() < loopTime; ++i ) // read keys
        {
            *reinterpret_cast< uint64_t* >( &key[0] ) = i - queueDepth;
            map[ key ];
        }
    }
    else // fetch + async read
    {
        for( i = 0; i < queueDepth; ++i ) // prefetch queueDepth keys
        {
            *reinterpret_cast< uint64_t* >( &key[0] ) = i;
            TEST( map.fetch( key, value_length ) );
        }

        for( ; i < wOps && clock.getTime64() < loopTime; ++i ) // read keys
        {
            *reinterpret_cast< uint64_t* >( &key[0] ) = i - queueDepth;
            map[ key ];

            *reinterpret_cast< uint64_t* >( &key[0] ) = i;
            TEST( map.fetch( key, value_length ));
        }

        for( uint64_t j = i - queueDepth; j <= i; ++j ) // drain fetched keys
        {
            *reinterpret_cast< uint64_t* >( &key[0] ) = j;
            map[ key ];
        }
    }

    const float readTime = clock.getTimef();
    const size_t rOps = i;
    value_bytes_transfer = i*value_length;
    float read_value_BW_MBs = 1000.0*value_bytes_transfer/(readTime*1024.0*1024.0);

    // fetch performance
    clock.reset();
    for( i = 0; i < wOps && clock.getTime64() < loopTime && i < LB_128KB; ++i )
    {
        *reinterpret_cast< uint64_t* >( &key[0] ) = i;
        TEST( map.fetch( key, value_length ) );
    }
    const float fetchTime = clock.getTimef();
    const size_t fOps = i;

    int nservers = useargs ? arg2_nservers : 0;
    std::cout << boost::format( "%6i, %7i, %7i, %7.2f, %7.2f, %7.2f, %8.3g, %8.3g")
        % queueDepth % nservers % value_length % (rOps/readTime) % (wOps/insertTime) % (fOps/fetchTime)
        % read_value_BW_MBs % write_value_BW_MBs
        << std::endl;

    // check contents of store (not all to save time on bigger tests)
    for( uint64_t j = 0; j < wOps /*&& clock.getTime64() < loopTime*/; ++j )
    {
        *reinterpret_cast< uint64_t* >( &key[0] ) = j;
        for (int q=0; q<value_qwords; ++q) *reinterpret_cast< uint64_t* >( &value[q*8] ) = i;
        std::string aval = map[key];
        TESTINFO( /*std::memcmp(&aval[0], &value[0], value_length) == 0,*/ aval.size()==value_length,
                  j << " = " << map[key].size());
    }

    // try to make sure there's nothing outstanding if we messed up in our test.
    map.flush();
}

void testGenericFailures()
{
    try
    {
        setup( "foobar://" );
    }
    catch( const std::runtime_error& )
    {
        return;
    }
    TESTINFO( false, "Missing exception" );
}

void testLevelDBFailures()
{
#ifdef LUNCHBOX_USE_LEVELDB
    try
    {
        setup( "leveldb:///doesnotexist/deadbeef/coffee" );
    }
    catch( const std::runtime_error& )
    {
        return;
    }
    TESTINFO( false, "Missing exception" );
#endif
}

int main( int, char* argv[] )
{
    const bool perfTest LB_UNUSED
        = std::string( argv[0] ).find( "perf_" ) != std::string::npos;

    // for scripting, we will pass 2 args
    if (argv[1] && argv[2]) {
        arg1_queuedepth = atoi(argv[1]);
        arg2_nservers   = atoi(argv[2]);
        useargs = true;
        std::cout << "Running for queue depth " << arg1_queuedepth << " on " << arg2_nservers << " servers" << std::endl;
    }

    try
    {
#ifdef __LUNCHBOX_USE_LEVELDB
        setup( "" );
        setup( "leveldb://" );
        setup( "leveldb://persistentMap2.leveldb" );
        read( "" );
        read( "leveldb://" );
        read( "leveldb://persistentMap2.leveldb" );
        if( perfTest )
            benchmark( "leveldb://", 0 );
#endif
#ifdef LUNCHBOX_USE_SKV
        FxLogger_Init( argv[0] );
        setup( "skv://" );
        read( "skv://" );
        if( perfTest )
        {
            std::cout << boost::format( "%6s, %7s, %7s, %7s, %7s, %7s, %8s, %8s")
                % "async" % "servers" % "vlength" % "read" % "write" % "fetch" % "r_BW" % "w_BW"
                << std::endl;

            size_t mindepth=0;
            size_t maxdepth=65536 + 1;
            if (useargs) {
               mindepth = arg1_queuedepth;
               maxdepth = arg1_queuedepth;
            }

            for (int vs=8; vs<65536+1; vs = vs << 1) {
                size_t queuedepth = mindepth;
                while (queuedepth<=maxdepth) {
                    // if queuedepth*value size gets too large, we have rdma memory pool problems
//                    if (i*vs < (32768*8192 + 1)) {
                        benchmark( "skv://", queuedepth, vs/8 );
//                    }
                    queuedepth = (queuedepth==0) ? 1 : (queuedepth<<1);
                }
            }
        }
#endif
    }
#ifdef LUNCHBOX_USE_LEVELDB
    catch( const leveldb::Status& status )
    {
        TESTINFO( !"exception", status.ToString( ));
    }
#endif
    catch( const std::runtime_error& error )
    {
#ifdef LUNCHBOX_USE_SKV
        if( error.what() !=
            std::string( "skv init failed: SKV_ERRNO_CONN_FAILED" ))
#endif
        {
            TESTINFO( !"exception", error.what( ));
        }
    }

    testGenericFailures();
    testLevelDBFailures();

    return EXIT_SUCCESS;
}
