This folder contains useful parts of MaxMind's GeoIP C API <http://www.maxmind.com/app/c>.

Header includes have been slightly changed.

The non-UTF-8 country name list has been removed.

A list of Windows GEOID and a conversion function (GeoIP_Win_GEOID_by_id) have been added.

An implementation of the pread function has been added for Windows.

The inet_pton implementation for Windows has been rewritten to use WSAStringToAddressA (the
getaddrinfo variant in the original file was causing crashes).
