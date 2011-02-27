package com.sigmusic.tacchi.tuio;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.Toast;


public class TuioTest extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
//        Log.d("asdf", stringFromJNI());
//        Log.d("testwin", testSUinputOpen());
        
//        final IWindowManager winman = new IWindowManager();
        MotionEvent e = MotionEvent.obtain(System.currentTimeMillis(), System.currentTimeMillis()+2, MotionEvent.ACTION_DOWN, 100, 100, 0);
//        WindowManagerService c;
//        winman.injectPointerEvent(e, true);
        doBindService();
        
    }
    
    private InstHackService mBoundService;
    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            mBoundService = ((InstHackService.LocalBinder)service).getService();
            // Tell the user about this for our demo.
            Toast.makeText(TuioTest.this,"Bound!",
                    Toast.LENGTH_SHORT).show();
        }

        public void onServiceDisconnected(ComponentName className) {
            mBoundService = null;
            Toast.makeText(TuioTest.this, "Disconnected",
                    Toast.LENGTH_SHORT).show();
        }
    };

    boolean mIsBound = false;
    void doBindService() {
    	Log.d("asdf","Bound!");
    	
        bindService(new Intent(TuioTest.this, 
                InstHackService.class), mConnection, Context.BIND_AUTO_CREATE);
        mIsBound = true;
    }

    
//    
//    public native String stringFromJNI();
//    public native String testSUinputOpen();
//    
//    static {
//    	System.loadLibrary("testmodule");
//    }
    
    

}