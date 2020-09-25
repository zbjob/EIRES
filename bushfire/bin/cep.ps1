#$query = $('bf-6.16','bf-1.7.16','bf-7.7_14','bf-7.7_14.16','bf-1.7.7_14.16','bf-1.6.16','bf-1.7_14.16')
#$query = $('bf-6.16','bf-1.7.16','bf-7.7_14.16','bf-1.6.16','bf-1.7_14.16')
#$query = $('bf-6.16','bf-7.7_14.16','bf-7.16','bf-1.6.16','bf-1.7.16','bf-1.7_14.16','bf-1.7.7_14','bf-6.7_14.16','bf-6.7.7_14','bf-7.714')
$query = $('bf-6.16','bf-7.7_14.16','bf-1.7_14.16','bf-1.7.7_14','bf-6.7_14.16','bf-6.7.7_14','bf-7.714')
#$query = $('bf-6.16','bf-7.7_14.16')
#$query = $('bf-7.16')

$PSDefaultParameterValues['Out-File:Encoding'] = 'utf8'
$location = 'county'
foreach ($q in $query){
    $cmdCep = "cep_match.exe -c ./patterns/" + $q + ".eql "+ "-f ./streams/" + $location + ".csv > ./results/"+$location+ "/result_" + $q  + ".txt 2>&1"
    echo $cmdCep
    iex $cmdCep
}