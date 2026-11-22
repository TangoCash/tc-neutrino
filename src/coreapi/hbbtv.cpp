/*
 * hbbtv.cpp - the media of an HbbTV application
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

#include "coreapi/hbbtv.h"

#include "coreapi/base/deps.h"
#include "coreapi/base/errors.h"

namespace coreapi
{

namespace hbbtv
{

Result<void> play(const std::string &url, int start_ms)
{
	if (url.compare(0, 7, "http://") != 0 && url.compare(0, 8, "https://") != 0)
		return fail(Status::InvalidArgument, ErrorCode::BadString, "the stream is not an http or https address");
	if (start_ms < 0)
		return fail(Status::InvalidArgument, ErrorCode::OutOfRange, "the start lies before the beginning");
	const Status s = hbbtvMedia().play(url, start_ms);
	if (s != Status::Ok)
		return fail(s, ErrorCode::PlaybackRunning, "the box could not start the stream");
	return ok();
}

Result<void> pause()
{
	hbbtvMedia().pause();
	return ok();
}

Result<void> resume()
{
	hbbtvMedia().resume();
	return ok();
}

Result<void> seek(int position_ms)
{
	if (position_ms < 0)
		return fail(Status::InvalidArgument, ErrorCode::OutOfRange, "the position lies before the beginning");
	hbbtvMedia().seek(position_ms);
	return ok();
}

Result<void> stop()
{
	hbbtvMedia().stop();
	return ok();
}

Result<void> window(int x, int y, int width, int height, int page_width, int page_height)
{
	if (page_width <= 0 || page_height <= 0)
		return fail(Status::InvalidArgument, ErrorCode::OutOfRange, "the page has no size");
	hbbtvMedia().window(x, y, width, height, page_width, page_height);
	return ok();
}

Result<Media> media()
{
	const HbbtvMediaState s = hbbtvMedia().status();
	Media m;
	m.state = s.state;
	m.position_ms = s.position_ms;
	m.duration_ms = s.duration_ms;
	return Result<Media>::success(m);
}

} // namespace hbbtv

} // namespace coreapi
