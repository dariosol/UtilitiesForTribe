name="$1"
extenctionA=".cpp"
extenctionB=".exe"
g++ -std=c++11 -g -pthread -m64 -I/usr/local/root/include $1$extenctionA  -L/usr/local/root/lib -lCore -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lpthread -Wl,-rpath,/opt/root/v6-14-06/lib -lm -ldl -lMinuit  -o $1$extenctionB
