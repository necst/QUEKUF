
for i in {3..10}
do
    python3 datasetGen.py $i 0.001 T
	mv Decoder_Dataset.txt ../Build/BUILD0.001/D$i 
done


for i in {3..10}
do
    python3 datasetGen.py $i 0.001 F
	mv Real_Decoder_Dataset.txt ../Build/BUILD0.001/D$i
done


for i in {3..8}
do
    python3 datasetGen.py $i 0.1 T
	mv Decoder_Dataset.txt ../Build/BUILD0.1/D$i
done


for i in {3..8}
do
    python3 datasetGen.py $i 0.1 F
	mv Real_Decoder_Dataset.txt ../Build/BUILD0.1/D$i
done

