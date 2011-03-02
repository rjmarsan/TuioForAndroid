package com.sigmusic.tacchi.tuio;

import java.util.ArrayList;

import processing.core.PApplet;
import android.content.Intent;
import android.os.Bundle;
import android.view.MotionEvent;


public class TuioTest extends PApplet {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        doBindService();
        
    }
    
//    private TuioService mBoundService;
//    private ServiceConnection mConnection = new ServiceConnection() {
//        public void onServiceConnected(ComponentName className, IBinder service) {
//        	if (service instanceof TuioService.LocalBinder) {
//        		mBoundService = ((TuioService.LocalBinder)service).getService();
//	            // Tell the user about this for our demo.
//	            Toast.makeText(TuioTest.this,"Bound!",
//	                    Toast.LENGTH_SHORT).show();
//	            Log.d("asff", "Service is bound!");
//	            mBoundService.startService(new Intent(TuioTest.this, 
//	            		TuioService.class));
//        	}
//        }
//
//        public void onServiceDisconnected(ComponentName className) {
//            mBoundService = null;
//            Toast.makeText(TuioTest.this, "Disconnected",
//                    Toast.LENGTH_SHORT).show();
//        }
//    };

//    boolean mIsBound = false;
    void doBindService() {
//    	Log.d("asdf","Bound!");
//    	
//        bindService(new Intent(TuioTest.this, 
//        		TuioService.class), mConnection, Context.BIND_AUTO_CREATE);
//        mIsBound = true;
    	Intent service = new Intent("com.sigmusic.tacchi.tuio.TUIO_SERVICE");
    	startService(service);
    }
    
    
//    float x = 0, y = 0;
//    public boolean surfaceTouchEvent(MotionEvent me) {
//    	x = me.getX();
//    	y = me.getY();
//    	return true;
//    }
//    
    public void draw() {
    	background(10,75,99);
    	ellipseMode(CENTER);
    	for (CursorStat c : cursors) {
    		infoCircle(c);
    	}
    }
    
    
    
    public class CursorStat {
    	public float x, y, siz, press;
    	public int id;
    	public CursorStat(float x, float y, float siz, int id, float press) {
    		this.x = x;
    		this.y = y;
    		this.siz = siz;
    		this.id = id;
    		this.press = press;
    	}
    }
    ArrayList<CursorStat> cursors = new ArrayList<CursorStat>();
    void infoCircle(CursorStat c) {
    	infoCircle(c.x, c.y, c.siz, c.id, c.press); 
    }

	void infoCircle(float x, float y, float siz, int id, float press) {
		// What is drawn on sceen when touched.
		float diameter = 30 * siz;
		noFill();
		ellipse(x, y, diameter, diameter);
		fill(0, 255, 0);
		ellipse(x, y, 8, 8);
		text(("ID:" + id + ", " + press + ", " + x + ", " + y), x - 128, y - 64);
	}

	// -----------------------------------------------------------------------------------------
	// Override Processing's surfaceTouchEvent, which will intercept all
	// screen touch events. This code only runs when the screen is touched.

	public boolean surfaceTouchEvent(MotionEvent me) {
//		cursors.clear();
		cursors = new ArrayList<CursorStat>();
		// Number of places on the screen being touched:
		int numPointers = me.getPointerCount();
		for (int i = 0; i < numPointers; i++) {
			int pointerId = me.getPointerId(i);
			float pressureId = me.getPressure(i);
			float x = me.getX(i);
			float y = me.getY(i);
			float siz = me.getSize(i);
			cursors.add(new CursorStat(x, y, siz, pointerId, pressureId));
		}
		// If you want the variables for motionX/motionY, mouseX/mouseY etc.
		// to work properly, you'll need to call super.surfaceTouchEvent().
		return true;
	}
   
	

    
    

}