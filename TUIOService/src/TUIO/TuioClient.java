/*
	TUIO Java backend - part of the reacTIVision project
	http://reactivision.sourceforge.net/

	Copyright (c) 2005-2009 Martin Kaltenbrunner <mkalten@iua.upf.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
package TUIO;

import com.illposed.osc.*;
import java.util.*;

/**
 * The TuioClient class is the central TUIO protocol decoder component. It provides a simple callback infrastructure using the {@link TuioListener} interface.
 * In order to receive and decode TUIO messages an instance of TuioClient needs to be created. The TuioClient instance then generates TUIO events
 * which are broadcasted to all registered classes that implement the {@link TuioListener} interface.<P> 
 * <code>
 * TuioClient client = new TuioClient();<br/>
 * client.addTuioListener(myTuioListener);<br/>
 * client.connect();<br/>
 * </code>
 *
 * @author Martin Kaltenbrunner
 * @version 1.4
 */ 
public class TuioClient implements OSCListener {
	
	private int port = 3333;
	private OSCPortIn oscPort;
	private boolean connected = false;
	private Hashtable<Long,TuioObject> objectList = new Hashtable<Long,TuioObject>();
	private Vector<Long> aliveObjectList = new Vector<Long>();
	private Vector<Long> newObjectList = new Vector<Long>();
	private Hashtable<Long,TuioCursor> cursorList = new Hashtable<Long,TuioCursor>();
	private Vector<Long> aliveCursorList = new Vector<Long>();
	private Vector<Long> newCursorList = new Vector<Long>();

	private Vector<TuioObject> frameObjects = new Vector<TuioObject>();
	private Vector<TuioCursor> frameCursors = new Vector<TuioCursor>();

	private Vector<TuioCursor> freeCursorList = new Vector<TuioCursor>();
	private int maxCursorID = -1;
	
	private long currentFrame = 0;
	private TuioTime currentTime;

	private Vector<TuioListener> listenerList = new Vector<TuioListener>();
	
	/**
	 * The default constructor creates a client that listens to the default TUIO port 3333
	 */
	public TuioClient() {}

	/**
	 * This constructor creates a client that listens to the provided port
	 *
	 * @param  port  the listening port number
	 */
	public TuioClient(int port) {
		this.port = port;
	}
		
	/**
	 * The TuioClient starts listening to TUIO messages on the configured UDP port
	 * All reveived TUIO messages are decoded and the resulting TUIO events are broadcasted to all registered TuioListeners
	 */
	public void connect() {

		TuioTime.initSession();
		currentTime = new TuioTime();
		currentTime.reset();
		
		try {
			oscPort = new OSCPortIn(port);
			oscPort.addListener("/tuio/2Dobj",this);
			oscPort.addListener("/tuio/2Dcur",this);
			oscPort.startListening();
			connected = true;
		} catch (Exception e) {
			System.out.println("TuioClient: failed to connect to port "+port);
			connected = false;
		}		
	}
	
	/**
	 * The TuioClient stops listening to TUIO messages on the configured UDP port
	 */
	public void disconnect() {
		oscPort.stopListening();
		try { Thread.sleep(100); }
		catch (Exception e) {};
		oscPort.close();
		connected = false;
	}

	/**
	 * Returns true if this TuioClient is currently connected.
	 * @return	true if this TuioClient is currently connected
	 */
	public boolean isConnected() { return connected; }

	/**
	 * Adds the provided TuioListener to the list of registered TUIO event listeners
	 *
	 * @param  listener  the TuioListener to add
	 */
	public void addTuioListener(TuioListener listener) {
		listenerList.addElement(listener);
	}
	
	/**
	 * Removes the provided TuioListener from the list of registered TUIO event listeners
	 *
	 * @param  listener  the TuioListener to remove
	 */
	public void removeTuioListener(TuioListener listener) {	
		listenerList.removeElement(listener);
	}

	/**
	 * Removes all TuioListener from the list of registered TUIO event listeners
	 */
	public void removeAllTuioListeners() {	
		listenerList.clear();
	}
	
	/**
	 * Returns a Vector of all currently active TuioObjects
	 *
	 * @return  a Vector of all currently active TuioObjects
	 */
	public Vector<TuioObject> getTuioObjects() {
		return new Vector<TuioObject>(objectList.values());
	}
	
	/**
	 * Returns a Vector of all currently active TuioCursors
	 *
	 * @return  a Vector of all currently active TuioCursors
	 */
	public Vector<TuioCursor> getTuioCursors() {
		return new Vector<TuioCursor>(cursorList.values());
	}	

	/**
	 * Returns the TuioObject corresponding to the provided Session ID
	 * or NULL if the Session ID does not refer to an active TuioObject
	 *
	 * @return  an active TuioObject corresponding to the provided Session ID or NULL
	 */
	public TuioObject getTuioObject(long s_id) {
		return objectList.get(s_id);
	}
	
	/**
	 * Returns the TuioCursor corresponding to the provided Session ID
	 * or NULL if the Session ID does not refer to an active TuioCursor
	 *
	 * @return  an active TuioCursor corresponding to the provided Session ID or NULL
	 */
	public TuioCursor getTuioCursor(long s_id) {
		return cursorList.get(s_id);
	}	

	/**
	 * The OSC callback method where all TUIO messages are received and decoded
	 * and where the TUIO event callbacks are dispatched
	 *
	 * @param  date	the time stamp of the OSC bundle
	 * @param  message	the received OSC message
	 */
	public void acceptMessage(Date date, OSCMessage message) {
	
		Object[] args = message.getArguments();
		String command = (String)args[0];
		String address = message.getAddress();

		if (address.equals("/tuio/2Dobj")) {

			if (command.equals("set")) {
				
				long s_id  = ((Integer)args[1]).longValue();
				int c_id  = ((Integer)args[2]).intValue();
				float xpos = ((Float)args[3]).floatValue();
				float ypos = ((Float)args[4]).floatValue();
				float angle = ((Float)args[5]).floatValue();
				float xspeed = ((Float)args[6]).floatValue();
				float yspeed = ((Float)args[7]).floatValue();
				float rspeed = ((Float)args[8]).floatValue();
				float maccel = ((Float)args[9]).floatValue();
				float raccel = ((Float)args[10]).floatValue();
				
				if (objectList.get(s_id) == null) {
				
					TuioObject addObject = new TuioObject(s_id,c_id,xpos,ypos,angle);
					frameObjects.addElement(addObject);
					
				} else {
				
					TuioObject tobj = objectList.get(s_id);
					if (tobj==null) return;
					if ((tobj.xpos!=xpos) || (tobj.ypos!=ypos) || (tobj.angle!=angle) || (tobj.x_speed!=xspeed) || (tobj.y_speed!=yspeed) || (tobj.rotation_speed!=rspeed) || (tobj.motion_accel!=maccel) || (tobj.rotation_accel!=raccel)) {
						
						TuioObject updateObject = new TuioObject(s_id,c_id,xpos,ypos,angle);
						updateObject.update(xpos,ypos,angle,xspeed,yspeed,rspeed,maccel,raccel);
						frameObjects.addElement(updateObject);
					}
				
				}
				
			} else if (command.equals("alive")) {
	
				newObjectList.clear();
				for (int i=1;i<args.length;i++) {
					// get the message content
					long s_id = ((Integer)args[i]).longValue();
					newObjectList.addElement(s_id);
					// reduce the object list to the lost objects
					if (aliveObjectList.contains(s_id))
						 aliveObjectList.removeElement(s_id);
				}
				
				// remove the remaining objects
				for (int i=0;i<aliveObjectList.size();i++) {
					TuioObject removeObject = objectList.get(aliveObjectList.elementAt(i));
					if (removeObject==null) continue;
					removeObject.remove(currentTime);
					frameObjects.addElement(removeObject);
				}
					
			} else if (command.equals("fseq")) {
				
				long fseq = ((Integer)args[1]).longValue();
				boolean lateFrame = false;
				
				if (fseq>0) {
					if (fseq>currentFrame) currentTime = TuioTime.getSessionTime();
					if ((fseq>=currentFrame) || ((currentFrame-fseq)>100)) currentFrame=fseq;
					else lateFrame = true;
				} else if (TuioTime.getSessionTime().subtract(currentTime).getTotalMilliseconds()>100) {
					currentTime = TuioTime.getSessionTime();
				}
				
				if (!lateFrame) {
					Enumeration<TuioObject> frameEnum = frameObjects.elements();
					while(frameEnum.hasMoreElements()) {
						TuioObject tobj = frameEnum.nextElement();
						
						switch (tobj.getTuioState()) {
							case TuioObject.TUIO_REMOVED:
								TuioObject removeObject = tobj;
								removeObject.remove(currentTime);
								for (int i=0;i<listenerList.size();i++) {
									TuioListener listener = (TuioListener)listenerList.elementAt(i);
									if (listener!=null) listener.removeTuioObject(removeObject);
								}								
								objectList.remove(removeObject.getSessionID());
								break;

							case TuioObject.TUIO_ADDED:
								TuioObject addObject = new TuioObject(currentTime,tobj.getSessionID(),tobj.getSymbolID(),tobj.getX(),tobj.getY(),tobj.getAngle());
								objectList.put(addObject.getSessionID(),addObject);
								for (int i=0;i<listenerList.size();i++) {
									TuioListener listener = (TuioListener)listenerList.elementAt(i);
									if (listener!=null) listener.addTuioObject(addObject);
								}
								break;
																
							default:
								TuioObject updateObject = objectList.get(tobj.getSessionID());
								if ( (tobj.getX()!=updateObject.getX() && tobj.getXSpeed()==0) || (tobj.getY()!=updateObject.getY() && tobj.getYSpeed()==0) )
									updateObject.update(currentTime,tobj.getX(),tobj.getY(),tobj.getAngle());
								else
									updateObject.update(currentTime,tobj.getX(),tobj.getY(),tobj.getAngle(),tobj.getXSpeed(),tobj.getYSpeed(),tobj.getRotationSpeed(),tobj.getMotionAccel(),tobj.getRotationAccel());

								for (int i=0;i<listenerList.size();i++) {
									TuioListener listener = (TuioListener)listenerList.elementAt(i);
									if (listener!=null) listener.updateTuioObject(updateObject);
								}
						}
					}
					
					for (int i=0;i<listenerList.size();i++) {
						TuioListener listener = (TuioListener)listenerList.elementAt(i);
						if (listener!=null) listener.refresh(new TuioTime(currentTime));
					}
					
					Vector<Long> buffer = aliveObjectList;
					aliveObjectList = newObjectList;
					// recycling the vector
					newObjectList = buffer;					
				}
				frameObjects.clear();
			}
		} else if (address.equals("/tuio/2Dcur")) {

			if (command.equals("set")) {

				long s_id  = ((Integer)args[1]).longValue();
				float xpos = ((Float)args[2]).floatValue();
				float ypos = ((Float)args[3]).floatValue();
				float xspeed = ((Float)args[4]).floatValue();
				float yspeed = ((Float)args[5]).floatValue();
				float maccel = ((Float)args[6]).floatValue();
				
				if (cursorList.get(s_id) == null) {
									
					TuioCursor addCursor = new TuioCursor(s_id, -1 ,xpos,ypos);
					frameCursors.addElement(addCursor);
					
				} else {
				
					TuioCursor tcur = cursorList.get(s_id);
					if (tcur==null) return;
					if ((tcur.xpos!=xpos) || (tcur.ypos!=ypos) || (tcur.x_speed!=xspeed) || (tcur.y_speed!=yspeed) || (tcur.motion_accel!=maccel)) {

						TuioCursor updateCursor = new TuioCursor(s_id,tcur.getCursorID(),xpos,ypos);
						updateCursor.update(xpos,ypos,xspeed,yspeed,maccel);
						frameCursors.addElement(updateCursor);
					}
				}
				
				//System.out.println("set cur " + s_id+" "+xpos+" "+ypos+" "+xspeed+" "+yspeed+" "+maccel);
				
			} else if (command.equals("alive")) {
	
				newCursorList.clear();
				for (int i=1;i<args.length;i++) {
					// get the message content
					long s_id = ((Integer)args[i]).longValue();
					newCursorList.addElement(s_id);
					// reduce the cursor list to the lost cursors
					if (aliveCursorList.contains(s_id)) 
						aliveCursorList.removeElement(s_id);
				}
				
				// remove the remaining cursors
				for (int i=0;i<aliveCursorList.size();i++) {
					TuioCursor removeCursor = cursorList.get(aliveCursorList.elementAt(i));
					if (removeCursor==null) continue;
					removeCursor.remove(currentTime);
					frameCursors.addElement(removeCursor);
				}
								
			} else if (command.equals("fseq")) {
				long fseq = ((Integer)args[1]).longValue();
				boolean lateFrame = false;
				
				if (fseq>0) {
					if (fseq>currentFrame) currentTime = TuioTime.getSessionTime();
					if ((fseq>=currentFrame) || ((currentFrame-fseq)>100)) currentFrame = fseq;
					else lateFrame = true;
				} else if (TuioTime.getSessionTime().subtract(currentTime).getTotalMilliseconds()>100) {
					currentTime = TuioTime.getSessionTime();
				}
				if (!lateFrame) {

					Enumeration<TuioCursor> frameEnum = frameCursors.elements();
					while(frameEnum.hasMoreElements()) {
						TuioCursor tcur = frameEnum.nextElement();
						
						switch (tcur.getTuioState()) {
							case TuioCursor.TUIO_REMOVED:
							
								TuioCursor removeCursor = tcur;
								removeCursor.remove(currentTime);
								
								for (int i=0;i<listenerList.size();i++) {
									TuioListener listener = (TuioListener)listenerList.elementAt(i);
									if (listener!=null) listener.removeTuioCursor(removeCursor);
								}

								cursorList.remove(removeCursor.getSessionID());

								if (removeCursor.getCursorID()==maxCursorID) {
									maxCursorID = -1;
									if (cursorList.size()>0) {
										Enumeration<TuioCursor> clist = cursorList.elements();
										while (clist.hasMoreElements()) {
											int c_id = clist.nextElement().getCursorID();
											if (c_id>maxCursorID) maxCursorID=c_id;
										}
										
										Enumeration<TuioCursor> flist = freeCursorList.elements();
										while (flist.hasMoreElements()) {
											int c_id = flist.nextElement().getCursorID();
											if (c_id>=maxCursorID) freeCursorList.removeElement(c_id);
										}
									} else freeCursorList.clear();
								} else if (removeCursor.getCursorID()<maxCursorID) {
									freeCursorList.addElement(removeCursor);
								}
								
								break;

							case TuioCursor.TUIO_ADDED:

								int c_id = cursorList.size();
								if ((cursorList.size()<=maxCursorID) && (freeCursorList.size()>0)) {
									TuioCursor closestCursor = freeCursorList.firstElement();
									Enumeration<TuioCursor> testList = freeCursorList.elements();
									while (testList.hasMoreElements()) {
										TuioCursor testCursor = testList.nextElement();
										if (testCursor.getDistance(tcur)<closestCursor.getDistance(tcur)) closestCursor = testCursor;
									}
									c_id = closestCursor.getCursorID();
									freeCursorList.removeElement(closestCursor);
								} else maxCursorID = c_id;		
								
								TuioCursor addCursor = new TuioCursor(currentTime,tcur.getSessionID(),c_id,tcur.getX(),tcur.getY());
								cursorList.put(addCursor.getSessionID(),addCursor);
								
								for (int i=0;i<listenerList.size();i++) {
									TuioListener listener = (TuioListener)listenerList.elementAt(i);
									if (listener!=null) listener.addTuioCursor(addCursor);
								}
								break;
								
							default:
								
								TuioCursor updateCursor = cursorList.get(tcur.getSessionID());
								if ( (tcur.getX()!=updateCursor.getX() && tcur.getXSpeed()==0) || (tcur.getY()!=updateCursor.getY() && tcur.getYSpeed()==0) )
									updateCursor.update(currentTime,tcur.getX(),tcur.getY());
								else 
									updateCursor.update(currentTime,tcur.getX(),tcur.getY(),tcur.getXSpeed(),tcur.getYSpeed(),tcur.getMotionAccel());
									
								for (int i=0;i<listenerList.size();i++) {
									TuioListener listener = (TuioListener)listenerList.elementAt(i);
									if (listener!=null) listener.updateTuioCursor(updateCursor);
								}
						}
					}
					
					for (int i=0;i<listenerList.size();i++) {
						TuioListener listener = (TuioListener)listenerList.elementAt(i);
						if (listener!=null) listener.refresh(new TuioTime(currentTime));
					}
					
					Vector<Long> buffer = aliveCursorList;
					aliveCursorList = newCursorList;
					// recycling the vector
					newCursorList = buffer;				
				}
				
				frameCursors.clear();
			} 

		}
	}
}
