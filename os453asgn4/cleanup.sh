service down secretkeeper
rm secretkeeper
make 
service up /dev/secrets/secretkeeper -dev /dev/Secret

