:while1	
	Debug\unittest
	if errorlevel != 0 goto break
	goto :while1

:break
echo "Your program has any problem.."
