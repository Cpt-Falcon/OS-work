#first test--does a diff with the regular unix shell and my program output
> outfileCmp
> outfile
echo ''
echo 'Make output:'
echo ''
make pipeit
echo ''
echo 'End make output'
echo ''
echo ''
echo 'calculating diff'
ls | sort -r > outfileCmp
./pipeit
diff outfileCmp outfile

#prevents the outfile from being written to in order to test error handling
rm outfileCmp
rm outfile
> outfileCmp
> outfile
chmod 411 outfile
chmod 411 outfileCmp
echo ''
echo 'Make output:'
echo ''
make pipeit
echo ''
echo 'End make output'
echo ''
echo ''
echo 'calculating diff--permission denied case'
ls | sort -r > outfileCmp
./pipeit
diff outfileCmp outfile
chmod 777 outfile
chmod 777 outfileCmp
echo ''
echo 'Make clean output'
echo ''
make clean
echo ''
echo 'End make output'
echo ''
echo ''
rm outfileCmp