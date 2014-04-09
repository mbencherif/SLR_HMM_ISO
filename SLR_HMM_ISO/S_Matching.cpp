#include "StdAfx.h"
#include "S_Matching.h"


S_CMatching::S_CMatching(void)
{
	m_pDhmm_test = new CHMM;
	m_pRecog = new CRecognition;
	handSegmentVideo.init();
	usefulFrameSize = 0;
}


S_CMatching::~S_CMatching(void)
{
}


void S_CMatching::loadModel(CString path)
{
	m_pDhmm_test->Init(path);
}


void S_CMatching::run(double **feature, int frameNum, int rank[], double score[])
{
	m_pRecog->GetHmmModel(m_pDhmm_test);
	double* pCadidateProb = new double [m_pRecog->m_pDhmm->m_nTotalHmmWord];
	int nWordNum, nRecogNum;
	m_pRecog->TestOneWordFromMemory(feature,frameNum,pCadidateProb,nWordNum,nRecogNum);


	//cout<<m_pRecog->m_pDhmm->m_pHmmWordIndex[nRecogNum]->Word<<endl;

	int sentenseNum = m_pRecog->m_pDhmm->m_nTotalHmmWord;
	scoreAndIndex* SI;
	SI = new scoreAndIndex[sentenseNum];
	for (int i=0; i<sentenseNum; i++)
	{
		SI[i].score = *(pCadidateProb + i);
		string temp(m_pRecog->m_pDhmm->m_pHmmWordIndex[i]->Word);
		string temp2(temp,1,4);
		SI[i].index = atoi(temp2.c_str());
		//cout<<SI[i].index<<" "<<SI[i].score<<endl;
	}
	sort(SI,SI+sentenseNum,comp2);

	for (int i=0; i<5; i++)
	{
		rank[i] = SI[i].index;
		score[i] = SI[i].score;
		//cout<<"Top "<<i<<": "<<rank[i]<<" "<<score[i]<<endl;
	}
}

bool S_CMatching::comp2(scoreAndIndex dis_1, scoreAndIndex dis_2)
{
	return dis_1.score > dis_2.score;
}

void S_CMatching::frameSelect_inMatch(int heightLimit, int leftY, int rightY)
{
	int heightThisLimit = min(leftY,rightY);
	if (heightThisLimit < heightLimit)
	{
		frameSelect.push_back(1);
		usefulFrameSize++;
	}
	else
	{
		frameSelect.push_back(0);
	}
}


void S_CMatching::readIndata(SLR_ST_Skeleton skeletonCurrent, Mat depthCurrent, IplImage* frameCurrent,int framID)
{
	if (framID == 0)
	{
// 		heightLimit = min(skeletonCurrent._2dPoint[7].y,
// 			skeletonCurrent._2dPoint[11].y) - 20;

		headPoint.x = skeletonCurrent._2dPoint[3].x;
		headPoint.y = skeletonCurrent._2dPoint[3].y;
		bool bHeadFound = handSegmentVideo.headDetection(
			frameCurrent,
			depthCurrent,
			headPoint);

		if(bHeadFound)
		{
			handSegmentVideo.colorClusterCv(handSegmentVideo.m_pHeadImage,3);
			handSegmentVideo.getFaceNeckRegion(frameCurrent,depthCurrent);
			handSegmentVideo.copyDepthMat(depthCurrent.clone());
		}
	}

// 	int heightThisLimit = min(skeletonCurrent._2dPoint[7].y,
// 		skeletonCurrent._2dPoint[11].y);
// 	if (heightThisLimit < heightLimit)
// 	{
// 		frameSelected == true;
// 		usefulFrameSize++;
// 	}
// 	else
// 	{
// 		frameSelected = false;
// 	}

	if (/*frameSelected*/ frameSelect[framID] == 1)
	{
		vSkeleton.push_back(skeletonCurrent);


		Posture posture;

		lPoint2.x = skeletonCurrent._2dPoint[7].x;
		lPoint2.y = skeletonCurrent._2dPoint[7].y;

		rPoint2.x = skeletonCurrent._2dPoint[11].x;
		rPoint2.y = skeletonCurrent._2dPoint[11].y;

		CvRect leftHand;
		CvRect rightHand;

		handSegmentVideo.kickHandsAll(frameCurrent,depthCurrent
		,lPoint2,rPoint2,posture,leftHand,rightHand);

		vPosture.push_back(posture);
	}
}


void S_CMatching::recognize(int rank[], double score[])
{
	myFeaExtraction.postureFeature(vPosture,handSegmentVideo);
	myFeaExtraction.SPFeature(vSkeleton);
	myFeaExtraction.PostureSP();


	//CString modelPath = "..\\model\\HmmData_50.dat";
	//loadModel(modelPath);
	run(myFeaExtraction.feature, myFeaExtraction.frameNum, rank, score);


	//Release resource.
	myFeaExtraction.release();
	for (int i=0; i<vPosture.size(); i++)
	{
		cvReleaseImage(&vPosture[i].leftHandImg);
		cvReleaseImage(&vPosture[i].rightHandImg);
	}
	vPosture.clear();
	vSkeleton.clear();
	frameSelect.clear();
}


void S_CMatching::patchRun(vector<SLR_ST_Skeleton> vSkeletonData, vector<Mat> vDepthData, vector<IplImage*> vColorData, 
	int rank[], double score[])
{
	SLR_ST_Skeleton skeletonCurrent;    //The 3 current data.
	Mat             depthCurrent;
	IplImage        *frameCurrent;

	int frameSize = vColorData.size();
	int heightLimit = min(vSkeletonData[0]._2dPoint[7].y,
		vSkeletonData[0]._2dPoint[11].y) - 20;

	for (int i=0; i<frameSize; i++)
	{
		frameSelect_inMatch(heightLimit, vSkeletonData[i]._2dPoint[7].y,
			vSkeletonData[i]._2dPoint[11].y);
	}

	for (int i=0; i<frameSize; i++)
	{
		skeletonCurrent = vSkeletonData[i];
		depthCurrent    = vDepthData[i];
		frameCurrent    = vColorData[i];
		int framID = i;
		readIndata(skeletonCurrent, depthCurrent, frameCurrent, framID);
	}

	recognize(rank, score);

}


void S_CMatching::ReadGallery(CString path)
{
	modelPath = path;
	loadModel(modelPath);
}
