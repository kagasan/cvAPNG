# cvAPNG  
C++とOpenCVでアニメーションpngを出力するためのクラス．  
次のようなコードが書けます．  

```cpp:sample.cpp
#include "cvAPNG.h"

int main(){

	cvAPNG apng;
	for (int i = 0; i < 256; i++){
		cv::Mat img(256, 256, CV_8UC3, cv::Scalar(255, 255, 255));
		cv::circle(img, cv::Point(i, i), i, cv::Scalar(i, i, i), -1);
		apng.push(img);
	}
	apng.write("result.png");
	
	return 0;
}
```

このコードの出力結果です.  
<img src="img/result.png">
