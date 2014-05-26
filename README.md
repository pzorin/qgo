## qGo 

qGo is a Go Client based on Qt 5.
It supports playing online at IGS-compatible servers (including some special tweaks for WING and LGS, also NNGS was reported to work) and locally against gnugo (or other GTP-compliant engines).
It also has rudimentary support for editing SGF files and parital support for CyberORO/WBaduk, Tygem, Tom, and eWeiqi (developers of these backends are currently inactive, everybody is welcome to take them over).

Go is an ancient Chinese board game. It's called "圍棋(Wei Qi)" in Chinese, "囲碁(Yi Go)" in Japanese, "바둑(Baduk)" in Korean.


Installation
------------
```sh
git clone https://github.com/pzorin/qgo.git
cd qgo
qmake
make
sudo make install
```
