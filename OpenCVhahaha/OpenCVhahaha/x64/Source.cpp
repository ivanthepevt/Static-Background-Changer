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
//We will create a couple of functions to calculate the median frame.
int computeMedian(vector<int> elements)
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


int main(int argc, char const* argv[])
{
    //setup 3 inputs including video, background and tolerance

    short tolerance;
    cout << "Type in your tolerance: ";
    cin >> tolerance;

    std::string video_file;
    std::string video_dire;
    cout << "Type in your video directory: ";
    cin >> video_dire;

    // Get video file
    if (argc > 1)
    {
        video_file = argv[1];
    }
    else
    {
        video_file = video_dire;
    }
    // Read background image
    std::string background_dire;
    cout << "Type in your background directory: ";
    cin >> background_dire;
    Mat background = imread(background_dire, IMREAD_COLOR);
    
    //ask for output file name
    std::string output_name;
    cout << "Type in your output file name: ";
    cin >> output_name;
    output_name += ".mp4";

    //extract audio using system call to ffmpeg, flag -y is to confirm overwriting
    std::string command_extract_audio = "ffmpeg -y -i " + video_dire + " -q:a 0 -map a temp_audio.mp3";
    
    if (system(command_extract_audio.c_str()) != 0)
    {
        cout << "ffmpeg failed huhuhuhu" << endl;
    }

    //Read video, has to perform when extracting audio with ffmpeg is done
    VideoCapture cap(video_file);
    if (!cap.isOpened())
        cerr << "Error opening video file\n";

    //Get the informations for output
    int frame_width = static_cast<int>(cap.get(3));
    int frame_height = static_cast<int>(cap.get(4));
    Size frame_size(frame_width, frame_height);
    int frame_per_second = cap.get(5);


    // Randomly select 25 frames
    default_random_engine generator;
    uniform_int_distribution<int>distribution(0,
        cap.get(CAP_PROP_FRAME_COUNT));

    vector<Mat> frames;
    Mat frame;

    for (int i = 0; i < 25; i++)
    {
        int fid = distribution(generator);
        cap.set(CAP_PROP_POS_FRAMES, fid);
        Mat frame;
        cap >> frame;
        if (frame.empty())
            continue;
        frames.push_back(frame);
    }
    // Calculate the median along the time axis
    cout << "Estimating background... ";
    Mat medianFrame = compute_median(frames);
    cout << "Estimating done!";

    // Display median frame (enable when debuging
    //imshow("frame", medianFrame);
    //waitKey(0);

    //  Reset frame number to 0
    cap.set(CAP_PROP_POS_FRAMES, 0);

    // Loop over all frames for preview
    /*
    while (1)
    {
        // Read frame
        cap >> frame;

        if (frame.empty())
            break;

        //FwB: Frame with Background
        Mat FwB = addbackground(tolerance, medianFrame, frame, background);

        imshow("frame", FwB);
        waitKey(20);
        if (key == 'q') {
            cout << "q key is pressed by the user. Stopping the video" << endl;
            break;
            }
    }

    cap.release();

    */

    //Export the video without audio
    VideoWriter output("temp_video.mp4", VideoWriter::fourcc('a', 'v', 'c', '1'), frame_per_second, frame_size);

    cout << "Exporting video... ";

    while (1)
    {
        // Read frame
        cap >> frame;

        if (frame.empty())
            break;

        //FwB: Frame with Background
        Mat FwB = addbackground(tolerance, medianFrame, frame, background);

        output.write(FwB);
        //imshow("frame", FwB);

        int key = waitKey(1);
        if (key == 'q') {
            cout << "q key is pressed by the user. Stopping the video" << endl;
            break;
        }
    }
    cout << "Done exporting!";

    cap.release();
    output.release();

    //system call command to join video and audio
    std::string command_join = "ffmpeg -y -i temp_video.mp4 -i temp_audio.mp3 -map 0:v -map 1:a -c:v copy -shortest " + output_name;
    if (system(command_join.c_str()) != 0)
    {
        cout << "ffmpeg failed huhuhuhu" << endl;
    }
    return 0;
}