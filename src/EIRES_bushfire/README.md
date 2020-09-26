# Prerequisites
CEP included `cparser` library as a tool for calculate expressions in predicates appearing in query. If it was not compiled, run: 

```
cd ./utils/cparse
make
```

Then, in root folder project, compiling CEP with makefile:
```
make
```
In some cases, if CEP not working well, please clean and recompile: 

```
make clean
make
```
# Running
## CEP in powershell windows
To test a lot of datasets or event patterns with minimal effort, run script **cep.ps1**:
```
cd bin
.\cep.ps1
```
Results are situated in `bin/results` folder.

## CEP in command line
CEP for bushfire application is built and located in `bin` folder. To run CEP, moving on: 

`cd bin` 

There are four availiable datasets in stream folder:

| Name      | Time     |
| :------------- | :----------: |
| California | 08-25/11/2018  |
| Woolsey   | 07-10/11/2018 |
| County   | 03-05/09/2019 |
| Kincade   | 23-24/10/2019 |

Running **cep_match.exe** for bushfire detection with options: 

* `-c` : event pattern
* `-f` : input stream

For example: 
```
cep_match.exe -c ./patterns/bf-6.16.eql -f ./streams/county.csv
```
We discovered two event patterns for bushfire situated in `bin/patterns` folder:
* day time: bf-7.7_14.16.eql
* night time: bf-6.16.eql

# CEP for weather
New datasets in `bin/streams` folder named `california_weather` using for integrate weather information in stream events. 
Weather information in file `california_weather` was built by averaging one by each of the pixels in boundary of event channel.

In previous CEP version, **Query.cpp** could not recognize *double* data type in event pattern. In addition, **PatternMatcher.cpp** also could not match with event having *double* data type. Therefore, I extended CEP bushfire for this requirement and enabled CEP to be availiable for both *int* and *double* data type, which adapted with the `california_weather` dataset. 

Now, CEP for `california_weather` dataset can be run as other datasets.



<!--- 
# Running with boost c++
`g++ -I C:/libs/boost/include/boost-1_72/  -o op.exe test_operator.cpp`
## C++ vscode
`"C:/libs/boost/include/boost-1_72/"`
## Terminal -> File
`cep_match.exe > result.txt 2>&1`
## Run Cparse
`g++ -o exp.exe test_exprtk.cpp ../utils/cparse/builtin-features.o ../utils/cparse/core-shunting-yard.o`
-->
