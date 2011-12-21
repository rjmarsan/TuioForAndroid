package com.sigmusic.tacchi.tuio;

import java.util.ArrayList;

import TUIO.TuioCursor;
import TUIO.TuioListener;
import TUIO.TuioObject;
import TUIO.TuioTime;
import android.view.KeyEvent;

public class TuioGestureControl  {
	private TuioService mService;
	private TuioAndroidClient mClient;
	
	private ArrayList<TuioListener> listeners = new ArrayList<TuioListener>();
	
	public TuioGestureControl(TuioService service, TuioAndroidClient client) {
		mService = service;
		mClient = client;
		listeners.add(new SwipeGestures(client.client, this));
	}
	
	public void startEvent() {
		mService.insertOverlay();
	}
	
	public void endEvent() {
		mService.removeOverlay();
	}
	
	public void fireHomeEvent() {
		mClient.cancelAll();
		mService.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_HOME));
		mService.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_HOME));
		mService.removeOverlay();
	}
	public void fireBackEvent() {
		mClient.cancelAll();
		mService.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BACK));
		mService.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_BACK));
		mService.removeOverlay();
	}
	public void fireMenuEvent() {
		mClient.cancelAll();
		mService.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_MENU));
		mService.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_MENU));
		mService.removeOverlay();
	}


}
