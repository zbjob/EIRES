echo "LoC with space lines and comments"
find src_PM_Distribution_test/ -name *.c -or -name *.h -or -name *.cpp -or -name *.py -or -name *.hpp| xargs cat | wc -l
echo "LoC without space lines"
find src_PM_Distribution_test/ -name *.c -or -name *.h -or -name *.cpp -or -name *.py -or -name *.hpp| xargs cat | grep -v ^$  | wc -l
echo "LoC without space lines and comments"
find src_PM_Distribution_test/ -name *.c -or -name *.h -or -name *.cpp -or -name *.py -or -name *.hpp| xargs cat | grep -v -e ^$ -e ^\s*\/\/.*$ | wc -l

