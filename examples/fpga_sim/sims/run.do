vsim -voptargs=+acc  work.tb
add wave -position insertpoint sim:/tb/*
run 10ms