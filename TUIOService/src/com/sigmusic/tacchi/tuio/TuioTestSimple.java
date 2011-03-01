package com.sigmusic.tacchi.tuio;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;


public class TuioTestSimple extends Activity implements OnTouchListener {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.setContentView(R.layout.main);
    	Intent service = new Intent("com.sigmusic.tacchi.tuio.TUIO_SERVICE");
    	startService(service);
    	
    	View v = this.findViewById(R.id.mainview);
    	v.setOnTouchListener(this);
    	

    	
    }
    
    
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		Log.d("TuioTest", "MotionEvent! "+event);
		return true;
	}
    
    
   
	

    
    

}