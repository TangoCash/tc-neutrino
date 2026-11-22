/*
 * hbbtv.h - the media of an HbbTV application
 *
 * Copyright (C) 2026 NI-Team
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __coreapi_hbbtv_h__
#define __coreapi_hbbtv_h__

#include "coreapi/base/result.h"

#include <string>

namespace coreapi
{

namespace hbbtv
{

/* The media of an HbbTV application: the browser shows the page above the
   picture and hands the streams of its <video> elements to the box, which
   plays them on its decoders below the page. */

struct Media
{
	// idle, starting, playing, paused, ended or error
	std::string state;
	int position_ms;
	int duration_ms;
};

// Live television stops; the stream opens in the background.
Result<void> play(const std::string &url, int start_ms);
Result<void> pause();
Result<void> resume();
Result<void> seek(int position_ms);
// Live television comes back. Nothing playing is fine.
Result<void> stop();
// Where the page shows the video, in the pixels of the page.
Result<void> window(int x, int y, int width, int height, int page_width, int page_height);
Result<Media> media();

} // namespace hbbtv

} // namespace coreapi

#endif
