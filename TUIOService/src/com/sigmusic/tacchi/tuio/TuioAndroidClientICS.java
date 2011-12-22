package com.sigmusic.tacchi.tuio;

import java.lang.reflect.Method;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.Queue;

import TUIO.TuioClient;
import TUIO.TuioContainer;
import TUIO.TuioCursor;
import TUIO.TuioListener;
import TUIO.TuioObject;
import TUIO.TuioTime;
import android.os.SystemClock;
import android.util.Log;
import android.view.MotionEvent;
import android.view.MotionEvent.PointerCoords;
import android.view.MotionEvent.PointerProperties;

public class TuioAndroidClientICS extends TuioAndroidClient implements TuioListener {
	public final static String TAG = "TuioAndroidClient";
	TuioClient client;
	TuioService callback;
	int width, height;
	int port;
	
	MotionEvent currentEvent;
	
	Queue<MotionEvent> events = new LinkedList<MotionEvent>();
	
	//public static final Method addBatch = getAddBatch();
	static Method getAddBatch() {
		try {
		return MotionEvent.class.getMethod("addBatch", long.class, float[].class, int.class);
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}
	}

	
	
	public TuioAndroidClientICS(TuioService callback,int port, int width, int height) {
		super(callback, port, width, height);
		client = new TuioClient(port);
		client.addTuioListener(this);
		this.callback = callback;
		this.port = port;
		this.width = width;
		this.height = height;
	}
	
	private static boolean running = true;
	private class EventSender extends Thread {
		public void run() {
			while (running) {
				int eventssize = 0;
				synchronized(events) {
					eventssize = events.size();
					if (eventssize > 0) {
						callback.sendMotionEvent(events.remove());
					} 	
				}
				if (eventssize <= 0) {
//						Log.d(TAG, "Sleepinf");
					try {
						Thread.sleep(50);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}
		}
	}
	
	private void sendUpdateEvent(MotionEvent update) {
//		if (update == null) return;
//		synchronized(events) {
//			events.add(update);
//		}
	}
	private void sendUpDownEvent(MotionEvent updownevent) {
//		if (updownevent == null) return;
//		synchronized(events) {
//			while (events.size() > 1 && events.peek().getAction() == MotionEvent.ACTION_MOVE) { //clear out all movement actions.
//				events.remove();
//			}
//			events.add(updownevent);
//		}
	}
	
	EventSender eventsender;
	
	public void addListener(TuioListener listener) {
		client.addTuioListener(listener);
	}
	
	public void cancelAll() {
		//TODO make this cancel everything
	}
	
	public void start() {
		Log.v(TAG, "Starting TUIO client on port:" +port);
		client.connect();
		eventsender = new EventSender();
		running = true;
		eventsender.start();
	}
	
	public void stop() {
		Log.v(TAG, "Stopping client");
		client.disconnect();
		running = false;
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
		
	
		
		PointerProperties[] pointerIds = new PointerProperties[totalcursors];
		int i =0;
		PointerCoords[] inData = new PointerCoords[totalcursors]; 
		for (TuioObject obj : client.getTuioObjects()) {
			if ((action != MotionEvent.ACTION_UP && action != MotionEvent.ACTION_POINTER_UP)  || obj.getSessionID() != id) { //we need to get rid of the removed cursor.
				pointerIds[i] = new PointerProperties();
				pointerIds[i].id = obj.getSymbolID();
				
				PointerCoords coord = new PointerCoords();
				
				coord.x = obj.getScreenX(width);
				coord.y = obj.getScreenY(height);
				coord.pressure = 1;//pressure
				coord.size = 0.1f; //size
				inData[i] = coord;
				i++;
			}
		}
		Collections.sort(client.getTuioCursors(), new Comparator<TuioCursor>() {
			public int compare(TuioCursor object1, TuioCursor object2) {
				if (object1.getCursorID() > object2.getCursorID()) {
					return 1;
				} else if (object1.getCursorID() <  object2.getCursorID()) {
					return -1;
				}
				return 0;
			}});
		for (TuioCursor obj : client.getTuioCursors()) {
			if ((action != MotionEvent.ACTION_UP && action != MotionEvent.ACTION_POINTER_UP) || obj.getSessionID() != id) { //we need to get rid of the removed cursor.
				pointerIds[i] = new PointerProperties();
				pointerIds[i].id = i;
				
				PointerCoords coord = new PointerCoords();
				
				coord.x = obj.getScreenX(width);
				coord.y = obj.getScreenY(height);
				coord.pressure = 1;//pressure
				coord.size = 0.1f; //size
				inData[i] = coord;
				i++;
			}

		}
		
		try {
			if (action == MotionEvent.ACTION_MOVE) {
				if (currentEvent.getPointerCount() == totalcursors) {
					synchronized(events) {
						if (events.size() < 1) { //if our queue is empty
							currentEvent.setAction(MotionEvent.ACTION_MOVE);
							//addBatch.invoke(currentEvent, totalms, inData, 0);
							currentEvent.addBatch(totalms, inData, 0);
							events.add(currentEvent);
						}
					}
				}
				else {
					currentEvent = MotionEvent.obtain(startms, SystemClock.uptimeMillis(), actionmasked, totalcursors, pointerIds, inData, 0, 0, 0.0f, 0.0f, 0, 0, 0, 0);
	//				return null; //EEK! something happened in the middle of us doing stuff.
					synchronized(events) {
						if (currentEvent != null)
							events.add(currentEvent);
					}
				}
			}
			else if (action == MotionEvent.ACTION_UP && totalcursors <= 1) {
	//			currentEvent.setAction(actionmasked); //nope that didn't fix it
				totalcursors = 1;
				/**long downTime, long eventTime,
            int action, int pointerCount, PointerProperties[] pointerProperties,
            PointerCoords[] pointerCoords, int metaState, int buttonState,
            float xPrecision, float yPrecision, int deviceId,
            int edgeFlags, int source, int flags**/
				currentEvent = MotionEvent.obtain(startms, SystemClock.uptimeMillis(), actionmasked, totalcursors, pointerIds, inData, 0, 0, 0.0f, 0.0f, 0, 0, 0, 0);
				synchronized(events) {
					if (currentEvent != null)
						events.add(currentEvent);
				}
			
			} else {
				currentEvent = MotionEvent.obtain(startms, SystemClock.uptimeMillis(), actionmasked, totalcursors, pointerIds, inData, 0, 0, 0.0f, 0.0f, 0, 0, 0, 0);
				synchronized(events) {
					if (currentEvent != null)
						events.add(currentEvent);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
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
//		callback.sendMotionEvent(me);
		this.sendUpDownEvent(me);
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
//		callback.sendMotionEvent(me);
		this.sendUpdateEvent(me);

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
		//callback.sendMotionEvent(me);
		this.sendUpDownEvent(me);
	}


	@Override
	public void refresh(TuioTime ftime) {		
	}


}
