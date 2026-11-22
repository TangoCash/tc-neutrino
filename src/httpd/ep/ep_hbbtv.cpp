/*
 * ep_hbbtv.cpp - the media of an HbbTV application
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

#include "httpd/endpoints.h"
#include "httpd/hbbtvtoken.h"

#include "httpd/endpoint.h"
#include "httpd/http.h"
#include "httpd/json.h"
#include "httpd/schema.h"
#include "httpd/auth.h"
#include "httpd/status.h"
#include "httpd/webconfig.h"

#include "coreapi/base/errors.h"
#include "coreapi/base/result.h"
#include "coreapi/hbbtv.h"

namespace httpd
{

namespace
{

/* The HbbTV browser on the box hands the streams of its pages here and drives
   their playback. Nobody else has a page drawn above the picture, so every
   route answers the box itself only. The routes that change something ask for
   Write like every other, and the browser presents a token of its own for that,
   scoped to these routes: the plugin starting it draws one (openHbbtvToken).
   The pages it shows come from the internet and reach 127.0.0.1 too, so being
   on the box is never enough on its own. */
const char kHbbtvScope[] = "hbbtv";

const FieldDesc kMediaFields[] = {
	HTTPD_MEMBER("state", FieldType::String,
		"idle: nothing playing; starting: the stream is being opened; playing; paused; "
		"ended: it played to its end; error: it could not be opened or played"),
	HTTPD_MEMBER("position", FieldType::Int, "milliseconds played, 0 while not playing or paused"),
	HTTPD_MEMBER("duration", FieldType::Int, "milliseconds in all, 0 when not known"),
};
const Schema kMediaSchema = { "hbbtv-media", HTTPD_FIELDS(kMediaFields) };

const Param kPlayParams[] = {
	HTTPD_BODY_REQUIRED_TEXT("url", "the stream, an http or https address of a file, an HLS playlist or a DASH manifest", 4096),
	HTTPD_BODY_IN("start", ParamType::Int, "where to start, milliseconds from the beginning; `0` (the default) for the beginning", 0, 86400000),
};

const Param kSeekParams[] = {
	HTTPD_BODY_REQUIRED_IN("position", ParamType::Int, "where to go, milliseconds from the beginning", 0, 86400000),
};

const Param kWindowParams[] = {
	HTTPD_BODY_REQUIRED_IN("x", ParamType::Int, "left edge of the video on the page", -8192, 8192),
	HTTPD_BODY_REQUIRED_IN("y", ParamType::Int, "top edge of the video on the page", -8192, 8192),
	HTTPD_BODY_REQUIRED_IN("width", ParamType::Int, "width of the video on the page, `0` when it shows none", 0, 8192),
	HTTPD_BODY_REQUIRED_IN("height", ParamType::Int, "height of the video on the page, `0` when it shows none", 0, 8192),
	HTTPD_BODY_REQUIRED_IN("page_width", ParamType::Int, "width of the page", 1, 8192),
	HTTPD_BODY_REQUIRED_IN("page_height", ParamType::Int, "height of the page", 1, 8192),
};

const RouteRefusal kLocalRefusals[] = {
	HTTPD_REFUSES(Denied, NotFromThisBox, "asked by another machine; only the box itself may ask"),
};

const RouteRefusal kPlayRefusals[] = {
	HTTPD_REFUSES(Denied, NotFromThisBox, "asked by another machine; only the box itself may ask"),
	HTTPD_REFUSES(InvalidArgument, BadString, "the stream is not an http or https address"),
};

bool fromThisBox(const Request &r, Response *refusal)
{
	/* a scoped token reaching here is held to its scope: this one, or none */
	if (!r.scope().empty() && r.scope() != kHbbtvScope)
	{
		*refusal = problemFor(coreapi::Error(coreapi::Status::Denied, coreapi::ErrorCode::NotFromThisBox,
			"the token is not one for the media of the HbbTV browser"));
		return false;
	}
	if (isLoopbackPeer(r.peer()))
		return true;
	*refusal = problemFor(coreapi::Error(coreapi::Status::Denied, coreapi::ErrorCode::NotFromThisBox,
		"only the box itself may drive the media of its HbbTV browser"));
	return false;
}

Response done(const coreapi::Result<void> &got)
{
	if (!got.ok())
		return problemFor(got.error());
	return noContent();
}

Response mediaNow(const Request &r)
{
	Response refusal;
	if (!fromThisBox(r, &refusal))
		return refusal;
	coreapi::Result<coreapi::hbbtv::Media> got = coreapi::hbbtv::media();
	if (!got.ok())
		return problemFor(got.error());
	const coreapi::hbbtv::Media &m = got.value();
	Response out = okJson();
	Json j(out.body, 96);
	j.beginObject();
	j.key("state");
	j.value(m.state);
	j.key("position");
	j.value(m.position_ms);
	j.key("duration");
	j.value(m.duration_ms);
	j.endObject();
	return out;
}

Response mediaPlay(const Request &r)
{
	Response refusal;
	if (!fromThisBox(r, &refusal))
		return refusal;
	return done(coreapi::hbbtv::play(r.asString("url"), r.has("start") ? (int) r.asInt("start") : 0));
}

Response mediaPause(const Request &r)
{
	Response refusal;
	if (!fromThisBox(r, &refusal))
		return refusal;
	return done(coreapi::hbbtv::pause());
}

Response mediaResume(const Request &r)
{
	Response refusal;
	if (!fromThisBox(r, &refusal))
		return refusal;
	return done(coreapi::hbbtv::resume());
}

Response mediaSeek(const Request &r)
{
	Response refusal;
	if (!fromThisBox(r, &refusal))
		return refusal;
	return done(coreapi::hbbtv::seek((int) r.asInt("position")));
}

Response mediaStop(const Request &r)
{
	Response refusal;
	if (!fromThisBox(r, &refusal))
		return refusal;
	return done(coreapi::hbbtv::stop());
}

Response mediaWindow(const Request &r)
{
	Response refusal;
	if (!fromThisBox(r, &refusal))
		return refusal;
	return done(coreapi::hbbtv::window((int) r.asInt("x"), (int) r.asInt("y"),
		(int) r.asInt("width"), (int) r.asInt("height"),
		(int) r.asInt("page_width"), (int) r.asInt("page_height")));
}

#define HBBTV_ONLY_LOCAL \
	"Only the box itself may ask: the HbbTV browser running on it hands the streams of its pages here, " \
	"with a token scoped to these routes that the plugin starting it draws.\n\n"

const Endpoint kHbbtvEndpoints[] = {
	{ Method::Get, "/api/v1/hbbtv", AuthLevel::Read,
	  "where the media of the HbbTV browser stands",
	  "Answers what the player under the HbbTV browser is doing: `state`, and `position` and `duration` "
	  "in milliseconds. The browser asks this about twice a second while a video plays and turns it into "
	  "the events of the page's video element.\n\n" HBBTV_ONLY_LOCAL
	  "**Refusals:**\n"
	  "- `403 not-from-this-box`: asked by another machine.\n\n"
	  "**Related:** `POST /api/v1/hbbtv/play`.",
	  NULL, 0, &kMediaSchema, &mediaNow, false,
	  Answers200, HTTPD_REFUSALS(kLocalRefusals) },
	{ Method::Post, "/api/v1/hbbtv/play", AuthLevel::Write,
	  "plays a stream of an HbbTV page below it",
	  "Stops live television and what the browser played before, and plays the stream on the box's "
	  "decoders, below the page the browser shows. Opening the stream goes on after the answer; "
	  "`GET /api/v1/hbbtv` says when it plays or why it could not.\n\n" HBBTV_ONLY_LOCAL
	  "**Refusals:**\n"
	  "- `400 bad-string`: `url` is not an http or https address.\n"
	  "- `403 not-from-this-box`: asked by another machine.\n\n"
	  "**Related:** `POST /api/v1/hbbtv/stop`, `GET /api/v1/hbbtv`.",
	  HTTPD_PARAMS(kPlayParams), NULL, &mediaPlay, true,
	  Answers204, HTTPD_REFUSALS(kPlayRefusals) },
	{ Method::Post, "/api/v1/hbbtv/pause", AuthLevel::Write,
	  "holds the stream of the HbbTV browser",
	  "Holds the stream where it is. Nothing playing is not refused.\n\n" HBBTV_ONLY_LOCAL
	  "**Refusals:**\n"
	  "- `403 not-from-this-box`: asked by another machine.\n\n"
	  "**Related:** `POST /api/v1/hbbtv/resume`.",
	  NULL, 0, NULL, &mediaPause, true,
	  Answers204, HTTPD_REFUSALS(kLocalRefusals) },
	{ Method::Post, "/api/v1/hbbtv/resume", AuthLevel::Write,
	  "plays the held stream of the HbbTV browser on",
	  "Plays a held stream on. Nothing held is not refused.\n\n" HBBTV_ONLY_LOCAL
	  "**Refusals:**\n"
	  "- `403 not-from-this-box`: asked by another machine.\n\n"
	  "**Related:** `POST /api/v1/hbbtv/pause`.",
	  NULL, 0, NULL, &mediaResume, true,
	  Answers204, HTTPD_REFUSALS(kLocalRefusals) },
	{ Method::Post, "/api/v1/hbbtv/seek", AuthLevel::Write,
	  "moves the stream of the HbbTV browser to a position",
	  "Moves the stream to `position`, in milliseconds from the beginning; while it is still being "
	  "opened, it starts there.\n\n" HBBTV_ONLY_LOCAL
	  "**Refusals:**\n"
	  "- `403 not-from-this-box`: asked by another machine.\n\n"
	  "**Related:** `GET /api/v1/hbbtv`.",
	  HTTPD_PARAMS(kSeekParams), NULL, &mediaSeek, true,
	  Answers204, HTTPD_REFUSALS(kLocalRefusals) },
	{ Method::Post, "/api/v1/hbbtv/stop", AuthLevel::Write,
	  "ends the stream of the HbbTV browser and gives live television back",
	  "Ends the stream and gives live television back, with the picture on the whole screen again. "
	  "Nothing playing is not refused; the plugin starting the browser sends this after it quit, in case "
	  "it left a stream behind.\n\n" HBBTV_ONLY_LOCAL
	  "**Refusals:**\n"
	  "- `403 not-from-this-box`: asked by another machine.\n\n"
	  "**Related:** `POST /api/v1/hbbtv/play`.",
	  NULL, 0, NULL, &mediaStop, true,
	  Answers204, HTTPD_REFUSALS(kLocalRefusals) },
	{ Method::Post, "/api/v1/hbbtv/window", AuthLevel::Write,
	  "puts the picture where the HbbTV page shows its video",
	  "Scales the picture to the place of the video element on the page, given in the pixels of the "
	  "page; a video on (almost) the whole page, or none, gets the whole screen.\n\n" HBBTV_ONLY_LOCAL
	  "**Refusals:**\n"
	  "- `403 not-from-this-box`: asked by another machine.\n\n"
	  "**Related:** `POST /api/v1/hbbtv/play`.",
	  HTTPD_PARAMS(kWindowParams), NULL, &mediaWindow, true,
	  Answers204, HTTPD_REFUSALS(kLocalRefusals) },
};

} // namespace

std::string openHbbtvToken()
{
	return openScopedToken(kHbbtvScope, AuthLevel::Write);
}

extern const RouteTable hbbtvTable = {
	HTTPD_TABLE("hbbtv", kHbbtvEndpoints)
};

} // namespace httpd
