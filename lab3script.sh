#PBS -q class
#PBS -l nodes=1
#PBS -l walltime=01:00:00
#PBS -N BronzeIVp3

#cd $PBS_O_WORKDIR

n=(20 100 300 600 1000) #(5 30 90)
txpow=(1 100 250 500) #(0.5 0.9 0.98)
tintense=(0.1 0.3 0.6 0.9)
rprot=('AODV' 'OLSR')



for i in ${n[@]}; do
	for j in ${txpow[@]}; do
		for k in ${tintense[@]} ; do
			for l in ${rprot[@]}; do
				./waf --run 'p3 --nWifi='${i}' --TrafficIntensity='${k}' --Rprot='${l}' --TxPower='${j}''
			done
		done
	done
done

# eof