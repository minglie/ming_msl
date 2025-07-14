        #setting parameter
work= work
output= ./
vsimbatch0= -do "run -all"


#commandbegin
all:compile vsim
lib:
	echo "start compile for Questasim10.6c"
	vlib $(work)
	vmap work $(work)
vlog :
	vlog  -f filelist.f -l $(output)/compile.log
compile: lib vlog
run:
	modelsim -do ./run.do
#make clean
clean:
	del *.wlf
	del vsim_stacktrace.vstf
	del transcript
	del compile.log
	del modelsim.ini
	rmdir /s /q work
