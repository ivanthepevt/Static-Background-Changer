#include "cMain.h"
#include <string.h>
#include <wx/txtstrm.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <random>


using namespace std;
using namespace cv;



struct BGRpixel {
	int B;
	int G;
	int R;
};

//Event table

wxBEGIN_EVENT_TABLE(cMain, wxFrame)
EVT_BUTTON(10001, OnButtonCliked1)
EVT_BUTTON(10002, OnButtonCliked2)
EVT_BUTTON(10004, OnButtonCliked3)
EVT_SCROLL_CHANGED(ScrollBar1)
wxEND_EVENT_TABLE()
//
string a = "                                                                                                    ";

//We will create a couple of functions to calculate the median frame.
int computeMedian(std::vector<int> elements)
{
	nth_element(elements.begin(), elements.begin() + elements.size() / 2, elements.end());

	//sort(elements.begin(),elements.end());
	return elements[elements.size() / 2];
}


cv::Mat compute_median(std::vector<cv::Mat> vec)
{
	// Note: Expects the image to be CV_8UC3
	cv::Mat medianImg(vec[0].rows, vec[0].cols, CV_8UC3, cv::Scalar(0, 0, 0));

	for (int row = 0; row < vec[0].rows; row++)
	{
		for (int col = 0; col < vec[0].cols; col++)
		{
			std::vector<int> elements_B;
			std::vector<int> elements_G;
			std::vector<int> elements_R;

			for (int imgNumber = 0; imgNumber < vec.size(); imgNumber++)
			{
				int B = vec[imgNumber].at<cv::Vec3b>(row, col)[0];
				int G = vec[imgNumber].at<cv::Vec3b>(row, col)[1];
				int R = vec[imgNumber].at<cv::Vec3b>(row, col)[2];

				elements_B.push_back(B);
				elements_G.push_back(G);
				elements_R.push_back(R);
			}

			medianImg.at<cv::Vec3b>(row, col)[0] = computeMedian(elements_B);
			medianImg.at<cv::Vec3b>(row, col)[1] = computeMedian(elements_G);
			medianImg.at<cv::Vec3b>(row, col)[2] = computeMedian(elements_R);
		}
	}
	return medianImg;
}
BGRpixel replacepixel(short tolerance, int Bframe, int Bmedian, int Bbg, int Gframe, int Gmedian, int Gbg, int Rframe, int Rmedian, int Rbg)
{
	BGRpixel pixvalue;
	//replacing the pixel with proper pixel for background remover, use later in addbackground()
	if (abs(Bframe - Bmedian) <= tolerance && abs(Gframe - Gmedian) <= tolerance && abs(Rframe - Rmedian) <= tolerance)
	{
		pixvalue.B = Bbg;
		pixvalue.G = Gbg;
		pixvalue.R = Rbg;
		//segment that match median should be subtracted
	}

	else
	{
		pixvalue.B = Bframe;
		pixvalue.G = Gframe;
		pixvalue.R = Rframe;
	}
	return pixvalue;
}

cv::Mat addbackground(short tolerance, cv::Mat median, cv::Mat frame, cv::Mat background)
{
	// To overwrite the backround to black in the frame
	// return the frame with background FwB
	cv::Mat FwB(median.rows, median.cols, CV_8UC3, cv::Scalar(0, 0, 0));

	// acess to data buffer of median background frame and FwB
	int cn = median.channels();

	uint8_t* pixelPtrmedian = (uint8_t*)median.data;
	Scalar_<uint8_t> bgrPixelmedian;

	uint8_t* pixelPtrbackground = (uint8_t*)background.data;
	Scalar_<uint8_t> bgrPixelbackground;

	uint8_t* pixelPtrframe = (uint8_t*)frame.data;
	Scalar_<uint8_t> bgrPixelframe;

	uint8_t* pixelPtrFwB = (uint8_t*)FwB.data;
	Scalar_<uint8_t> bgrPixelFwB;


	for (int row = 0; row < median.rows; row++)
	{
		for (int col = 0; col < median.cols; col++)
		{
			//frame
			bgrPixelframe.val[0] = pixelPtrframe[row * frame.cols * cn + col * cn + 0]; // B
			bgrPixelframe.val[1] = pixelPtrframe[row * frame.cols * cn + col * cn + 1]; // G
			bgrPixelframe.val[2] = pixelPtrframe[row * frame.cols * cn + col * cn + 2]; // R
			//median
			bgrPixelmedian.val[0] = pixelPtrmedian[row * median.cols * cn + col * cn + 0]; // B
			bgrPixelmedian.val[1] = pixelPtrmedian[row * median.cols * cn + col * cn + 1]; // G
			bgrPixelmedian.val[2] = pixelPtrmedian[row * median.cols * cn + col * cn + 2]; // R
			//background
			bgrPixelbackground.val[0] = pixelPtrbackground[row * background.cols * cn + col * cn + 0]; // B
			bgrPixelbackground.val[1] = pixelPtrbackground[row * background.cols * cn + col * cn + 1]; // G
			bgrPixelbackground.val[2] = pixelPtrbackground[row * background.cols * cn + col * cn + 2]; // R
			//compute the FwB
			BGRpixel mypix = replacepixel(tolerance, bgrPixelframe.val[0], bgrPixelmedian.val[0], bgrPixelbackground.val[0], bgrPixelframe.val[1], bgrPixelmedian.val[1], bgrPixelbackground.val[1], bgrPixelframe.val[2], bgrPixelmedian.val[2], bgrPixelbackground.val[2]);
			FwB.at<cv::Vec3b>(row, col)[0] = mypix.B;
			FwB.at<cv::Vec3b>(row, col)[1] = mypix.G;
			FwB.at<cv::Vec3b>(row, col)[2] = mypix.R;
			//long approach but improve performance as pointer pont directly to the value
		}
	}
	return FwB;
}

// Function to reverse a string
void reverseStr(string& str)
{
	int n = str.length();

	// Swap character starting from two
	// corners
	for (int i = 0; i < n / 2; i++)
		swap(str[i], str[n - i - 1]);
}



cMain::cMain() : wxFrame(nullptr, wxID_ANY, a+"Static Background Changer", wxPoint(30, 30), wxSize(800, 600), wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX))
{

	//Definition for button
	m_btn1 = new wxButton(this, 10001, "Open Video", wxPoint(40, 30), wxSize(150, 35));
	m_btn2 = new wxButton(this, 10002, "New Bkgnd", wxPoint(40, 100), wxSize(150, 35));
	m_btn3 = new wxButton(this, wxID_ANY, "20", wxPoint(130, 485), wxSize(50, 50));
	m_btn4 = new wxButton(this, 10004, "Export", wxPoint(600, 485), wxSize(150, 50));

	//Text boxes
	m_txt1 = new wxTextCtrl(this, wxID_ANY, "", wxPoint(220, 30), wxSize(560, 35), wxALIGN_LEFT);
	m_txt2 = new wxTextCtrl(this, wxID_ANY, "", wxPoint(220, 100), wxSize(560, 35), wxALIGN_LEFT);

	//Slider Bar
	m_slider = new wxSlider(this, 10003, 20,0,100,wxPoint(200, 500), wxSize(400,20));

	//Texts: Original, Preview, Threshold
	m_text1 = new wxStaticText(this, wxID_ANY, "Tolerance", wxPoint(50, 500), wxSize(50, 30));
	m_text2 = new wxStaticText(this, wxID_ANY, "Original Video", wxPoint(150, 175), wxSize(100, 25));
	m_text3 = new wxStaticText(this, wxID_ANY, "Preview", wxPoint(545, 175), wxSize(100, 25));
	//Change the font size, apply for all
	wxFont font = m_text1->GetFont();
	font.SetPointSize(font.GetPointSize() + 3);
	m_text1->SetFont(font);
	m_btn1->SetFont(font);
	m_btn2->SetFont(font);
	m_btn3->SetFont(font);
	m_btn4->SetFont(font);
	m_txt1->SetFont(font);
	m_txt2->SetFont(font);
	//Change the font privately for Original and Preview
	font.SetPointSize(font.GetPointSize() + 2);
	m_text2->SetFont(font);
	m_text3->SetFont(font);

	//Add image handler
	wxImage::AddHandler(png);
	wxImage::AddHandler(jpeg);

	//Original Image
	bitmap1 = new wxBitmap("huy.png", wxBITMAP_TYPE_PNG);
	wxImage img1 = bitmap1->ConvertToImage();    //create image from bitmap
	img1.Rescale(368, 207); //rescale the image
	wxBitmap NewBitmap(img1);    //convert back to bitmap
	image1 = new wxStaticBitmap(this, wxID_ANY,img1, wxPoint(20, 200), wxSize(368, 207));

	//Preview Image
	bitmap2 = new wxBitmap("huy.png", wxBITMAP_TYPE_PNG);   //create a bitmap
	wxImage img2 = bitmap2->ConvertToImage();   //create an image from the bitmap
	img2.Rescale(368, 207);   //rescale the image
	wxBitmap newBitmap(img2);
	image2 = new wxStaticBitmap(this, wxID_ANY, img2, wxPoint(400, 200), wxSize(368, 207));


	//add video in the future
	//I dont think we need this - Hiep
	//m_video1 = new wxMediaCtrl(this, wxID_ANY, "video.mp4", wxPoint(40, 200), wxSize(300, 240), wxMC_NO_AUTORESIZE);
	//m_video2 = new wxMediaCtrl(this, wxID_ANY, "Knight.mp4", wxPoint(450, 200), wxSize(300, 240), wxMC_NO_AUTORESIZE);
	//m_video1->ShowPlayerControls();
	//m_video2->ShowPlayerControls();

	//Initialize File Diaglog
	m_dialog1 = new wxFileDialog(this,"","", "Hello", "*.*", wxFD_OPEN,wxPoint(30,30),wxSize(200,200));
	m_dialog2 = new wxFileDialog(this, "", "", "Hello", "*.*", wxFD_OPEN, wxPoint(30, 30), wxSize(200, 200));
	m_dialog3 = new wxFileDialog(this, "", "", "Hello", "*.*", wxFD_SAVE, wxPoint(30, 30), wxSize(200, 200));




	//////////////////////////////////////////////////ALGORITHM/////////////////////////////////////////////////////////////
	//Get Open file name

	//////////////////////////////////////////////////////////////////END OF ALGORITHM/////////////////////////////////////
}

Mat background;
Mat medianFrame;
vector<Mat> frames;
Mat frame;
string global_video_name = "";
string global_background_name = "";
VideoCapture globalcap;
int tolerance;

cMain::~cMain()
{

}

void cMain::OnButtonCliked1(wxCommandEvent& evt)
{
	cout << "Button 1 clicked" << endl;
	//Click Open file, open file diaglog
	m_dialog1->ShowModal();
	//Print the path into the box 1
	m_txt1->SetLabel(m_dialog1->GetPath());

	///////////////////////////////////Get Input Video////////////////////////////////////
	if ((m_txt1->GetLabel()).ToStdString() != "")
	{
		string video_dir = (m_txt1->GetLabel()).ToStdString();
		//extract audio using system call to ffmpeg, flag -y is to confirm overwriting
		std::string command_extract_audio = "ffmpeg -y -i " + video_dir + " -q:a 0 -map a temp_audio.mp3";

		if (system(command_extract_audio.c_str()) != 0)
		{
			cout << "ffmpeg failed huhuhuhu" << endl;
		}
		global_video_name = video_dir;
	}

	//////////////////////////////////////Given the file names, now we can display the original videos and background videos/////////////////////////////////
	if (global_video_name != "")
	{
		//Read Video
		VideoCapture cap(global_video_name);
		globalcap = cap;
		//Read Background
		//Mat background = imread(global_background_name, IMREAD_COLOR);
		//Error handling will be fixed later if I have time, now just assume it will always work.
		//Randomly select 25 frames
		default_random_engine generator;
		uniform_int_distribution<int>distribution(0, cap.get(CAP_PROP_FRAME_COUNT));

		for (int i = 0; i < 25; i++)
		{
			int	fid = distribution(generator);
			cap.set(CAP_PROP_POS_FRAMES, fid);
			Mat frame;
			cap >> frame;
			if (frame.empty())
				continue;
			frames.push_back(frame);
		}


		//Calculate the median along the time axis
		medianFrame = compute_median(frames);

		//
		// Display median frame
		//imshow("frame", medianFrame);
		//waitKey(0);
	}
	evt.Skip();
	// we obtained the median frame
}


void cMain::OnButtonCliked2(wxCommandEvent& evt)
{
	//cout << "btn2 clicked" << endl;
	m_dialog2->ShowModal();
	m_txt2->SetLabel(m_dialog2->GetPath());
	if ((m_txt2->GetLabel()).ToStdString() != "")
	{
		string image_dir = (m_txt2->GetLabel()).ToStdString();
		background = imread(image_dir, IMREAD_COLOR);
		}
	//Draw an image to check for debug
	//imshow("background", background);
    //waitKey(0);
	evt.Skip();
}

void cMain::ScrollBar1(wxScrollEvent& evt)
{
	int a = m_slider->GetValue();
	std::string b = std::to_string(a);
	m_btn3->SetLabel(b);
	///////////////////////////////////////For viewing original video and image when slide the slider/////////////////////
	tolerance = m_slider->GetValue();
	//assign value to tolerance
	/////////////////////////////////////Get Background Image///////////////////////////////////////
	if ((m_txt2->GetLabel()).ToStdString() != "")
	{

		//Write preview

		//Set frame number to 150
		globalcap.set(CAP_PROP_POS_FRAMES, 150);
		globalcap >> frame;
		imwrite("original.png", frame);
		Mat FwB = addbackground(tolerance, medianFrame, frame, background);
		imwrite("preview.png", FwB);
		//Display preview
		bitmap1 = new wxBitmap("original.png", wxBITMAP_TYPE_PNG);
		wxImage img1 = bitmap1->ConvertToImage();    //create image from bitmap
		img1.Rescale(368, 207); //rescale the image
		wxBitmap NewBitmap(img1);    //convert back to bitmap
		image1 = new wxStaticBitmap(this, wxID_ANY, img1, wxPoint(20, 200), wxSize(368, 207));

		bitmap2 = new wxBitmap("preview.png", wxBITMAP_TYPE_PNG);   //create a bitmap
		wxImage img2 = bitmap2->ConvertToImage();   //create an image from the bitmap
		img2.Rescale(368, 207);   //rescale the image
		wxBitmap newBitmap(img2);
		image2 = new wxStaticBitmap(this, wxID_ANY, img2, wxPoint(400, 200), wxSize(368, 207));

	}

	evt.Skip();
}

void cMain::OnButtonCliked3(wxCommandEvent& evt)
{
	int frame_width = static_cast<int>(globalcap.get(3));
	int frame_height = static_cast<int>(globalcap.get(4));
	Size frame_size(frame_width, frame_height);
	int frame_per_second = globalcap.get(5);
	//Click Open Save file
	m_dialog3->ShowModal();
	string save_dir = m_dialog3->GetPath().ToStdString();
	m_btn4->SetLabel("Exporting...");
	//Export a muted video as temporary
	globalcap.set(CAP_PROP_POS_FRAMES, 0);
	VideoWriter output("temp_video.mp4", VideoWriter::fourcc('a', 'v', 'c', '1'), frame_per_second, frame_size);
	while (1)
	{
		// Read frame
		globalcap >> frame;

		if (frame.empty())
			break;

		//FwB: Frame with Background
		Mat FwB = addbackground(tolerance, medianFrame, frame, background);

		output.write(FwB);
		//imshow("frame", FwB);
		// why show frame? - long
		// Sorry, it is for debugging only. I forgot to comment it - Hiep

	}

	globalcap.release();
	output.release();

	//system call command to join video and audio
	std::string command_join = "ffmpeg -y -i temp_video.mp4 -i temp_audio.mp3 -map 0:v -map 1:a -c:v copy -shortest " + save_dir;
	if (system(command_join.c_str()) != 0)
	{
		cout << "ffmpeg failed huhuhuhu" << endl;
	}
	m_btn4->SetLabel("Exported!");
	evt.Skip();
}
