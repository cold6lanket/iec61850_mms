%%----------------------------------------------------------------
%% IEC 61850 MMS client Erlang wrapper.
%% Mirrors the C request handlers in c_src/client/src/iec61850_mms_client.c.
%%----------------------------------------------------------------
-module(iec61850_mms_client).

%% Control API
-export([start_link/1, start_link/2,
         set_log_level/2,
         stop/1]).

%% Protocol API (must align with C 'on_request' router)
-export([info/1, info/2,
         browse/2, browse/3,
         connect/2, connect/3,
         read_items/2, read_items/3,
         write_items/2, write_items/3]).

-define(CONNECT_TIMEOUT, 30000).
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
            {unix, _}     -> atom_to_list(?MODULE);
            {win32, _}    -> atom_to_list(?MODULE) ++ ".exe"
        end,
    LocalSourcePath = filename:absname(Source),
    PrivSourcePath = Dir ++ "/" ++ Source,
    SourcePath =
        case filelib:is_file(LocalSourcePath) of
            true -> unicode:characters_to_binary(LocalSourcePath);
            false ->
                case filelib:is_file(PrivSourcePath) of
                    true -> unicode:characters_to_binary(PrivSourcePath);
                    false -> unicode:characters_to_binary(LocalSourcePath)
                end
        end,
    eport_c:start_link(SourcePath, Name, Options).

stop(Pid) ->
    eport_c:stop(Pid).

set_log_level(Pid, Level) ->
    eport_c:set_log_level(Pid, Level).

%%----------------------------------------------------------------
%% Protocol API
%%----------------------------------------------------------------
info(Pid) ->
    info(Pid, undefined).
info(Pid, Timeout) ->
    eport_c:request(Pid, <<"info">>, #{}, Timeout).

browse(Pid, Params) ->
    browse(Pid, Params, undefined).
browse(Pid, Params, Timeout) ->
    eport_c:request(Pid, <<"browse">>, Params, Timeout).

connect(Pid, Params) ->
    connect(Pid, Params, ?CONNECT_TIMEOUT).
connect(Pid, Params, Timeout) ->
    case eport_c:request(Pid, <<"connect">>, Params, Timeout) of
        {ok, <<"ok">>} -> ok;
        Error -> Error
    end.

read_items(Pid, Items) ->
    read_items(Pid, Items, undefined).
read_items(Pid, Items, Timeout) ->
    eport_c:request(Pid, <<"read_items">>, Items, Timeout).

write_items(Pid, Items) ->
    write_items(Pid, Items, undefined).
write_items(Pid, Items, Timeout) ->
    eport_c:request(Pid, <<"write_items">>, Items, Timeout).
