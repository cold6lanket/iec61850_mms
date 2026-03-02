%%----------------------------------------------------------------
%% IEC 61850 MMS server Erlang wrapper.
%% Routes Erlang calls through eport_c to C entrypoints defined in
%% c_src/server/src/iec61850_mms_server.c.
%%----------------------------------------------------------------
-module(iec61850_mms_server).

%% Control API
-export([
    start_link/1, start_link/2,
    set_log_level/2,
    stop/1
]).

%% Protocol API
-export([
    server_start/2,
    write_items/2, write_items/3,
    read_items/2, read_items/3
]).

-define(RESPONSE_TIMEOUT, 5000).

%%----------------------------------------------------------------
%% Control API
%%----------------------------------------------------------------
start_link(Name) ->
    start_link(Name, #{response_timeout => ?RESPONSE_TIMEOUT}).

start_link(Name, Options) ->
    Dir = code:priv_dir(iec61850_mms),
    Source =
        case os:type() of
            {unix, linux} -> atom_to_list(?MODULE);
            {win32, _}    -> atom_to_list(?MODULE) ++ ".exe"
        end,
    SourcePath = unicode:characters_to_binary(Dir ++ "/" ++ Source),
    eport_c:start_link(SourcePath, Name, Options).

stop(Pid) ->
    eport_c:stop(Pid).

set_log_level(Pid, Level) ->
    eport_c:set_log_level(Pid, Level).

%%----------------------------------------------------------------
%% Protocol API
%%----------------------------------------------------------------
server_start(Pid, Params) ->
    case eport_c:request(Pid, <<"server_start">>, Params) of
        {ok, <<"ok">>} -> ok;
        Error -> Error
    end.

%% Items map:
%% #{ <<"LD0/LLN0.Mod.stVal">> => #{ type => <<"Boolean">>, value => true } }
write_items(Pid, Items) ->
    write_items(Pid, Items, undefined).
write_items(Pid, Items, Timeout) ->
    eport_c:request(Pid, <<"write_items">>, Items, Timeout).

%% Items list:
%% [<<"LD0/LLN0.Mod.stVal">>, <<"LD0/MMXU1.A.phsA.cVal.mag.f">>]
read_items(Pid, Items) ->
    read_items(Pid, Items, undefined).
read_items(Pid, Items, Timeout) ->
    eport_c:request(Pid, <<"read_items">>, Items, Timeout).

