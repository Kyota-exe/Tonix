#pragma once

enum class Error
{
    None = 0,
    Exists = 1019,
    IsDirectory = 1029,
    NoFile = 1043,
    InvalidArgument = 1026,
    InvalidFileDescriptor = 1081,
    NotTerminal = 1058,
    NotDirectory = 1053
};
