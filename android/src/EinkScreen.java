package org.xcsoar;


public class EinkScreen {

	public static int UpdateMode;
	public static int UpdateModeInterval;
	public static int RefreshNumber = 0;
	
	public static void PrepareController() {
			if (UpdateMode == 1) {
				if (UpdateModeInterval > 0 && RefreshNumber >= UpdateModeInterval) {
					RefreshNumber = 0;
					N2EpdController.setGL16Mode(1);
				} else {
					N2EpdController.setGL16Mode(0);
					if (UpdateModeInterval > 0) {
						RefreshNumber++;
					}	
				}	
			}	
	}
	public static void ResetController(int dept) {
			if (UpdateMode == 1) {
				N2EpdController.setGL16Mode(dept);
				RefreshNumber = 0;
			}	
	}
}
