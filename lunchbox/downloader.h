
/* Copyright (c) 2013, Stefan.Eilemann@epfl.ch
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

#ifndef LUNCHBOX_DOWNLOADER_H
#define LUNCHBOX_DOWNLOADER_H

#include <lunchbox/api.h>
#include <lunchbox/types.h>
#include <lunchbox/thread.h>         // thread-safety macros

namespace lunchbox
{
namespace detail { class Downloader; }

/** A C++ class to handle one downloader instance. */
class Downloader : public NonCopyable
{
public:
    /** Construct a new downloader instance. */
    LUNCHBOX_API Downloader();

    /**
     * Construct a new, named downloader instance.
     *
     * @param from the plugin registry.
     * @param name the name of the downloader.
     */
    LUNCHBOX_API Downloader( PluginRegistry& from, const uint32_t name );

    /**
     * Construct a new, auto-selected downloader instance.
     * @sa choose() for parameters.
     */
    LUNCHBOX_API Downloader( PluginRegistry& from,const uint32_t internalFormat,
                             const float minQuality, const bool ignoreAlpha,
                             const uint64_t capabilities,const GLEWContext* gl);

    /** Destruct the downloader. */
    LUNCHBOX_API virtual ~Downloader();

    /** @return true if the instance is usable. */
    LUNCHBOX_API bool isGood() const;

     /** @return true if the instance is usable and has the given name. */
    LUNCHBOX_API bool uses( const uint32_t name ) const;

    /**
     * @return true if the download supports the given parameters.
     * @version 1.7.1
     */
    LUNCHBOX_API bool supports( const uint32_t inputToken, const bool noAlpha,
                                const uint64_t capabilities ) const;

    /**
     * Find the best downloader in all plugins for the given parameters.
     *
     * This convenience method searches all compressors in all plugins to find
     * the downloader which supports the given parameters and provides the
     * highest speed.
     *
     * @param from the plugin registry to choose from.
     * @param internalFormat the input token type to the downloader.
     * @param minQuality the minimum quality.
     * @param ignoreAlpha true if the downloader may drop the alpha channel.
     * @param capabilities the capabilities required by the downloader.
     * @param gl the OpenGL function table.
     */
    static LUNCHBOX_API uint32_t choose( const PluginRegistry& from,
                                         const uint32_t internalFormat,
                                         const float minQuality,
                                         const bool ignoreAlpha,
                                         const uint64_t capabilities,
                                         const GLEWContext* gl );

    /** @return the information about the allocated downloader. */
    LUNCHBOX_API const EqCompressorInfo& getInfo() const;

    /** Swap the two downloaders instances. */
    LUNCHBOX_API void swap( Downloader& other );

    /** Reset to EQ_COMPRESSOR_NONE. */
    LUNCHBOX_API void clear();

    /**
     * Start downloading data from the GPU to the CPU
     *
     * @param buffer data destination buffer.
     * @param inDims the dimensions of the input data.
     * @param flags capability flags for the compression.
     * @param outDims return value for the dimensions of the output data.
     * @param source the source texture name, or 0 for framebuffer.
     * @param gl the OpenGL function table.
     * @return true if finish is needed, false if a synchronous download was
     *         done.
     * @version 1.7.1
     */
    LUNCHBOX_API bool start( void** buffer, const uint64_t inDims[4],
                             const uint64_t flags, uint64_t outDims[4],
                             const unsigned source, const GLEWContext* gl );

    /**
     * Finish download data from the GPU to the CPU
     *
     * @param buffer data destination buffer.
     * @param inDims the dimensions of the input data.
     * @param flags capability flags for the compression.
     * @param outDims return value for the dimensions of the output data.
     * @param gl the OpenGL function table.
     * @version 1.7.1
     */
    LUNCHBOX_API void finish( void** buffer, const uint64_t inDims[4],
                              const uint64_t flags, uint64_t outDims[4],
                              const GLEWContext* gl );

    LUNCHBOX_API const GLEWContext* glewGetContext() const; //!< @internal

private:
    detail::Downloader* impl_;
    LB_TS_VAR( _thread );
};
}
#endif  // LUNCHBOX_DOWNLOADER_H