#pragma once

enum class Error : int64_t
{
    None = 0,
    Exists = 1019,
    IsDirectory = 1029,
    NoFile = 1043,
    InvalidArgument = 1026,
    InvalidFileDescriptor = 1081,
    NotTerminal = 1058,
    NotDirectory = 1053,
    IsPipe = 1069,
    Fault = 1020,
    BadFileDescriptor = 1081,
    BadRange = 3,
    NoChildren = 1012,
};
