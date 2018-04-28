:: Name: build-appveyor.cmd
:: Purpose: Support readable multi-line polly.py build commands
:: Copyright 2016-2017 Elucideye, Inc.
::
:: Multi-line commands are not currently supported directly in appveyor.yml files
::
:: See: http://stackoverflow.com/a/37647169

echo POLLY_ROOT %POLLY_ROOT%

python %POLLY_ROOT%\bin\polly.py ^
--verbose ^
--archive drishti_hunter_test ^
--config "%1%" ^
--toolchain "%2%" ^
--jobs 2 ^
--test ^
--fwd HUNTER_USE_CACHE_SERVERS=YES ^
HUNTER_DISABLE_BUILDS=NO ^
HUNTER_CONFIGURATION_TYPES="%1%" ^
HUNTER_SUPPRESS_LIST_OF_FILES=ON
