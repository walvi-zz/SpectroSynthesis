#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/audio/GenNode.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "AudioDrawUtils.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CinderProjectApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
    
    float calcFreqToPixel(float freq);
    float calcPixelToFreq(int pixel);
    float calcAmpForLine(int pLine);
    void threadLoad();
    void processColumn();
    
    vector<int> getRelLines(Surface surface);
    
    gl::Texture2dRef mTex;
    Surface surface;
    
    audio::GainNodeRef masterGain;
    audio::AddNodeRef masterAdd;
    audio::MonitorNodeRef mMonitor;
    
    std::vector<audio::GenNodeRef> freqs;
    std::vector<audio::GainNodeRef> gains;
    
    std::thread mThread;
    
    int iter = 0;
    float sTime, eTime, dur;
};

void CinderProjectApp::setup()
{
    try {
        surface = loadImage(loadAsset("KPAudio_pixelbeat.bmp"));
        mTex = gl::Texture::create(surface);
        
    } catch (Exception) {
        std::cout << "No Asset found" << std::endl;
    }
    
    auto ctx = audio::Context::master();
    
    masterGain = ctx->makeNode(new audio::GainNode);
    masterAdd = ctx->makeNode(new audio::AddNode);
    mMonitor = ctx->makeNode( new audio::MonitorNode );
    
    vector<int> lines = getRelLines(surface);
    
    
    int numOsc = 710;
    int maxFreq = 8000;
    int minFreq = 17;
    int tempFreq = minFreq;
    int height = surface.getHeight();
    
    //logarithmische Verteilung im Spektralraum
    //nochmal überarbeiten!!
    for(int i=numOsc; i>=0; i--){
        
        float tempFreq = (float)i/(float)numOsc;
        tempFreq*=9;
        
        tempFreq = pow(exp(1),tempFreq); // e^9 === 8000Hz
                                        //e^10 ==== 22000Hz
        
        audio::GenNodeRef genNode = ctx->makeNode(new audio::GenSineNode(tempFreq+100.0f));
        
        std::cout << tempFreq+50.0f << std::endl;
        
        
        //Max distance between oscillators for staying in positiv freqs
//        float dis = surface.getHeight() / numOsc;
        
        audio::GainNodeRef gainNode = ctx->makeNode(new audio::GainNode);
        gainNode->setValue(0.0f);
        
        genNode >> gainNode >> masterGain;
        genNode->enable();
        
        freqs.push_back(genNode);
        gains.push_back(gainNode);
    }
    masterGain->setValue(1.0f/freqs.size());
    
    masterGain >> mMonitor;
    masterGain >> ctx->getOutput();
    
    ctx->enable();
    
//    std::string s = ctx->printGraphToString();
//    ctx->getOutput()->enableClipDetection();
    
    mThread = std::thread(&CinderProjectApp::threadLoad, this);

}

void CinderProjectApp::update()
{
    
}

void CinderProjectApp::threadLoad(){
    
    ThreadSetup threadsetup;
    
    sTime=getElapsedSeconds();
    
    while(1){
        processColumn();
        
        if(iter > surface.getWidth()){
            std::cout << "Song is over!" << std::endl;
        }
        //should sleep for 1/100 of a sec - 0.01sec - 10ms - 10,000mus - 10,000,000ns
        std::this_thread::sleep_for(std::chrono::microseconds(2000));
        
        iter++;
    }
}

void CinderProjectApp::processColumn(){
    
    float dis = (float)surface.getHeight() / gains.size();
    
    for(int i=0; i<gains.size(); i++){
        
        if(iter > surface.getWidth() ){
            
            gains.at(i)->setValue(0.0f);
        }
        else {
        
            int pLine = dis*i;//calcFreqToPixel(freqs.at(i)->getFreq());
            
            float amp = calcAmpForLine(pLine);
            
            gains.at(i)->setValue(amp);
        }
        
    }
    if(iter % 100 == 0 && iter != 0){
        
        eTime = getElapsedSeconds();
        dur = eTime - sTime;
        
        sTime = eTime;
        
        std::cout << "Iter: " << iter << " duration: " << dur << std::endl;
    }

    
}
//Returns all pixel Lines that have at least one pixel that is not black
vector<int> CinderProjectApp::getRelLines(Surface surface){
    
    vector<int> rLines;
    
    for(int i=0; i<surface.getHeight(); i++){
        for(int j=0; j<surface.getWidth(); j++){
            
            float pixel = surface.getPixel(ivec2 (j,i)).r / 255.0f;
            
            if(pixel > 0){
                rLines.push_back(i);
                break;
            }
        }
    }
    return rLines;
    
}

//Calculates the Amplitude of each Pixel (i.e. the brightness) in the entered Line
float CinderProjectApp::calcAmpForLine(int pLine){
    
    auto p = surface.getPixel(ivec2 (iter, pLine));
    
    //Calculating Brightness - approx.
    float g = p.r + p.g + p.b;
    g *= 0.333f;

    float amp = g / 255.0f;

    return amp;
    
}

//takes in a frequency and returns the corresponding y-value of the spectrogramm
float CinderProjectApp::calcFreqToPixel(float freq){
    
    //Pixels per Frequency
    float ppF = surface.getHeight() / 20000.0f;
    
    float pixel = surface.getHeight() - (freq * ppF);
    
    return pixel;
}

// takes in the y-value of a pixel and returns the corresponding frequency
float CinderProjectApp::calcPixelToFreq(int pixel){
    
    //Frequencies per Pixel
    float fpP = 20000.0f / surface.getHeight();
    
    float freq = (surface.getHeight() - pixel) * fpP;
    
    std::cout << "Osc at: " << pixel << " , freq: " << freq << endl;
    
    return freq;
}

void CinderProjectApp::draw()
{
	gl::clear();
    
    if( mMonitor && mMonitor->getNumConnectedInputs() ) {
//        Rectf scopeRect( getWindowWidth() - 210, 10, getWindowWidth() - 10, 110 );
        Rectf scopeRect( 10, 10, getWindowWidth()-10, 310 );
        drawAudioBuffer( mMonitor->getBuffer(), scopeRect, true );
    }
//    gl::draw(mTex, getWindowBounds());


}

CINDER_APP( CinderProjectApp, RendererGl )
