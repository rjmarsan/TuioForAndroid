package com.sigmusic.tacchi.tuio;

import TUIO.TuioClient;
import TUIO.TuioContainer;
import TUIO.TuioCursor;
import TUIO.TuioListener;
import TUIO.TuioObject;
import TUIO.TuioTime;
import android.util.Log;
import android.view.MotionEvent;

public class TuioAndroidClient implements TuioListener {
	public final static String TAG = "TuioAndroidClient";
	TuioClient client;
	TuioService callback;
	int width, height;
	
	MotionEvent currentEvent;
	
	public TuioAndroidClient(TuioService callback, int width, int height) {
		client = new TuioClient();
		client.addTuioListener(this);
		this.callback = callback;
		this.width = width;
		this.height = height;
	}
	
	public void addListener(TuioListener listener) {
		client.addTuioListener(listener);
	}
	
	public void cancelAll() {
		//TODO make this cancel everything
	}
	
	public void start() {
		Log.v(TAG, "Starting TUIO client on port:" +3333);
		client.connect();
	}
	
	public void stop() {
		Log.v(TAG, "Stopping client");
		client.disconnect();
	}
	
	
	
	private int getNumCursors() {
		return client.getTuioCursors().size() + client.getTuioObjects().size();
	}
	private MotionEvent makeOrUpdateMotionEvent(TuioContainer point, long id, int action) {
		long startms = point.getStartTime().getTotalMilliseconds();
		long totalms = point.getTuioTime().getTotalMilliseconds();
		int totalcursors = getNumCursors();
		if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
			totalcursors -= 1;
			if (totalcursors <= 0) { //no cursors is a special case
				id = -1;
				totalcursors = 1;
				action = MotionEvent.ACTION_UP; //no more ACTION_POINTER_UP. need to be serious.
			}
		}
		int actionmasked = action ;//|( 0x1 << (7+id) & 0xff00);
		
		Log.d("TuioEvent", "TuioContainer: "+point.toString()+" startms: "+startms+" totalms: "+totalms+" cursors: "+totalcursors+" id: "+id+ " action: "+action+" Action masked: "+actionmasked);
		
	
		
		int[] pointerIds = new int[totalcursors];
		int i =0;
		float[] inData = new float[totalcursors * 4]; // 4 = MotionEvent.NUM_SAMPLE_DATA;
		for (TuioObject obj : client.getTuioObjects()) {
			if ((action != MotionEvent.ACTION_UP && action != MotionEvent.ACTION_POINTER_UP)  || obj.getSessionID() != id) { //we need to get rid of the removed cursor.
				pointerIds[i] = obj.getSymbolID();
				
				inData[i*4] = obj.getScreenX(width);
				inData[i*4+1] = obj.getScreenY(height);
				inData[i*4+2] = 1;//pressure
				inData[i*4+3] = 0.1f; //size
				i++;
			}
		}
		for (TuioCursor obj : client.getTuioCursors()) {
			if ((action != MotionEvent.ACTION_UP && action != MotionEvent.ACTION_POINTER_UP) || obj.getSessionID() != id) { //we need to get rid of the removed cursor.
				pointerIds[i] = obj.getCursorID();
				inData[i*4] = obj.getScreenX(width);
				inData[i*4+1] = obj.getScreenY(height);
				inData[i*4+2] = 1;//pressure
				inData[i*4+3] = 0.1f; //size
				i++;
			}

		}
		

		if (action == MotionEvent.ACTION_MOVE) {
			if (currentEvent.getPointerCount() == totalcursors) {
				currentEvent.setAction(MotionEvent.ACTION_MOVE);
				currentEvent.addBatch(totalms, inData, 0);
			}
			else {
//				return null; //EEK! something happened in the middle of us doing stuff.
				currentEvent = MotionEvent.obtainNano(startms, totalms, System.nanoTime(), actionmasked, totalcursors, pointerIds, inData, 0, 0, 0, 0, 0);

			}
		}
		else if (action == MotionEvent.ACTION_UP && totalcursors <= 1) {
//			currentEvent.setAction(actionmasked); //nope that didn't fix it
			totalcursors = 0;
			currentEvent = MotionEvent.obtainNano(startms, totalms, System.nanoTime(), actionmasked, totalcursors, pointerIds, inData, 0, 0, 0, 0, 0);
		} else {
			currentEvent = MotionEvent.obtainNano(startms, totalms, System.nanoTime(), actionmasked, totalcursors, pointerIds, inData, 0, 0, 0, 0, 0);
		}
		
		
		
		
		
		return currentEvent;
	}
	
	

	@Override
	public void addTuioCursor(TuioCursor tcur) {
		Log.d(TAG, "Cursor down: "+tcur.toString());
		addTuioThing(tcur, tcur.getSessionID());
	}

	@Override
	public void addTuioObject(TuioObject tobj) {
		Log.d(TAG, "Fiducial down: "+tobj.toString());
		addTuioThing(tobj, tobj.getSessionID());
	}
	
	
	private void addTuioThing(TuioContainer point, long id) {
//		Log.d(TAG, "forwarding");
		int event = (id == id) ? MotionEvent.ACTION_DOWN : MotionEvent.ACTION_POINTER_DOWN;
		MotionEvent me = makeOrUpdateMotionEvent(point, id, event);
		callback.sendMotionEvent(me);
	}
	

	@Override
	public void updateTuioCursor(TuioCursor tcur) {
		Log.d(TAG, "Cursor moved: "+tcur.toString());
		updateTuioThing(tcur, tcur.getSessionID());
	}

	@Override
	public void updateTuioObject(TuioObject tobj) {
		Log.d(TAG, "fiducial moved: "+tobj.toString());
		updateTuioThing(tobj, tobj.getSessionID());
	}
	
	private void updateTuioThing(TuioContainer point, long id) {
		int event = MotionEvent.ACTION_MOVE;
		MotionEvent me = makeOrUpdateMotionEvent(point, id, event);
		callback.sendMotionEvent(me);

	}

	@Override
	public void removeTuioCursor(TuioCursor tcur) {
		Log.d(TAG, "Cursor up: "+tcur.toString());
		removeTuioThing(tcur, tcur.getSessionID());
	}

	@Override
	public void removeTuioObject(TuioObject tobj) {
		Log.d(TAG, "Fiducal up: "+tobj.toString());
		removeTuioThing(tobj, tobj.getSessionID());
	}
	
	private void removeTuioThing(TuioContainer point, long id) {
//		Log.d(TAG, "forwarding");
		int event = (id == 0) ? MotionEvent.ACTION_UP : MotionEvent.ACTION_POINTER_UP;
		MotionEvent me = makeOrUpdateMotionEvent(point, id, event);
		callback.sendMotionEvent(me);
	}


	@Override
	public void refresh(TuioTime ftime) {		
	}


}
