package com.sigmusic.tacchi.tuio;

import processing.core.PApplet;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.Toast;


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
    
    
    float x = 0, y = 0;
    public boolean surfaceTouchEvent(MotionEvent me) {
    	x = me.getX();
    	y = me.getY();
    	return true;
    }
    
    public void draw() {
    	background(10,55,99);
    	rectMode(CENTER);
    	rect(x, y, 15, 15);
    }
    
    
    
   
	

    
    

}