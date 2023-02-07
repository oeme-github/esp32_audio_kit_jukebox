

$a = Get-Content .\backtrace.txt -Delimiter " "
$ENV="esp32dev"
$PROJECT="firmware"

foreach ( $ADDRESS in $a )
{
    ~/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-addr2line -pfiaC -e .pio/build/$ENV/$PROJECT.elf $ADDRESS
}

