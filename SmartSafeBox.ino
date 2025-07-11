#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP32Servo.h>
#include <Preferences.h>

// --- RemoteXY Library and Connection Settings ---
#define REMOTEXY_MODE__WIFI_CLOUD
#include <RemoteXY.h>

#define REMOTEXY_WIFI_SSID "WIFI_SSID"          // The name of the WiFi network to connect to
#define REMOTEXY_WIFI_PASSWORD "WIFI_PASSWORD"  // The password of the WiFi network to connect to
#define REMOTEXY_CLOUD_SERVER "CLOUD_SERVER"    // We get the server part from the RemoteXY My Cloud tokens section
#define REMOTEXY_CLOUD_PORT 1234                // We get the device port part from the RemoteXY My Cloud tokens section
#define REMOTEXY_CLOUD_TOKEN "CLOUD_TOKEN"      // We get the token part from the RemoteXY My Cloud tokens section

// RemoteXY GUI Configuration - This part should be copied from the RemoteXY EDITOR!"
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] = { 255, 15, 0, 0, 0, 89, 0, 19, 0, 0, 0, 0, 24, 1, 200, 84, 1, 1, 4, 0,
                            7, 8, 36, 183, 19, 102, 192, 6, 16, 203, 14, 129, 41, 6, 114, 10, 64, 78, 69, 110,
                            116, 101, 114, 32, 84, 104, 101, 32, 83, 109, 97, 114, 116, 32, 83, 97, 102, 101, 32, 66,
                            111, 120, 0, 129, 77, 22, 42, 9, 64, 94, 80, 97, 115, 115, 119, 111, 114, 100, 58, 0,
                            1, 66, 59, 73, 22, 3, 45, 31, 67, 111, 110, 102, 105, 114, 109, 0 };

struct {
  char edit_passWord[14];
  uint8_t button_confirm;
  uint8_t connect_flag;
} RemoteXY;
#pragma pack(pop)

// --- Telegram Bot Settings ---
// Since BOT_TOKEN is a C-style string, we cannot use isEmpty().
// If the bot token is left empty, bot.sendMessage() will not work anyway.

#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"  // // Token received from BotFather
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
long lastUpdateID = 0;

// --- Password and Security Settings ---
String ownerPassword;
String secondUserPassword;
const int MIN_PASSWORD_LENGTH = 8;
const int MAX_PASSWORD_LENGTH = 13;
const int MAX_WRONG_ATTEMPTS_BEFORE_ALARM = 2;
int wrongAttempts = 0;

// --- Hardware Pin Definitions ---
#define BUZZER_PIN 2
#define SERVO_PIN 5
Servo safeBoxServo;

// --- SafeBox Status Variables ---
bool isAlarmActive = false;
bool isSafeBoxOpen = false;
unsigned long safeBoxOpenTime = 0;
const long AUTO_CLOSE_DELAY_MS = 30000;  // 30 saniye

// --- Telegram Chat IDs ---
const String OWNER_CHAT_ID = "owner_Id";             // Chat ID of the SafeBox Owner
const String SECOND_USER_CHAT_ID = "secondUser_ID";  // Chat ID of the Second User

// --- Persistent Data Storage ---
Preferences preferences;

// --- Function Declarations ---
void handleNewMessages(int numNewMessages);
void openSafeBoxDoor(const String& openedBy);
void closeSafeBoxDoor();
void activateAlarm();
void deactivateAlarm();
void savePasswords();
void loadPasswords();
void handleRemoteXYPasswordEntry();
bool isValidPasswordLength(int length);

void setup() {
  Serial.begin(115200);
  delay(100);

  RemoteXY_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  safeBoxServo.attach(SERVO_PIN);
  closeSafeBoxDoor();

  preferences.begin("safeBox_Data", false);
  loadPasswords();

  // Initial setup or assigning passwords
  if (ownerPassword.isEmpty()) {
    ownerPassword = "OwnerPass8";
    Serial.println("Initial setup: Owner password set as '" + ownerPassword + "'.");
  }

  if (secondUserPassword.isEmpty()) {
    secondUserPassword = "UserPass8";
    Serial.println("Initial setup: Second user password set as '" + secondUserPassword + "'.");
  }
  savePasswords();

  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  Serial.println("Smart Safe Box System Started!");
}

void loop() {
  RemoteXY_Handler();

  handleRemoteXYPasswordEntry();

  // --- Telegram Bot Updates ---
  const long BOT_UPDATE_INTERVAL_MS = 2500;
  static unsigned long lastBotUpdate = 0;

  if (millis() - lastBotUpdate >= BOT_UPDATE_INTERVAL_MS) {
    int numNewMessages = bot.getUpdates(lastUpdateID + 1);
    if (numNewMessages > 0) {
      handleNewMessages(numNewMessages);
      lastUpdateID = bot.messages[numNewMessages - 1].update_id;
    }
    lastBotUpdate = millis();
  }

  // --- SafeBox Door Auto-Close ---
  if (isSafeBoxOpen && millis() - safeBoxOpenTime >= AUTO_CLOSE_DELAY_MS) {
    closeSafeBoxDoor();
    // The check was removed since bot.sendMessage won’t work if BOT_TOKEN is empty anyway.
    // However, the case of CHAT_IDs being empty is still being checked.
    if (!OWNER_CHAT_ID.isEmpty()) {
      bot.sendMessage(OWNER_CHAT_ID, "Smart Safe Box: The Safe Box was closed automatically.", "");
    }
    if (!SECOND_USER_CHAT_ID.isEmpty()) {
      bot.sendMessage(SECOND_USER_CHAT_ID, "Smart Safe Box: The Safe Box was closed automatically.", "");
    }
  }
}

// Password length check
bool isValidPasswordLength(int length) {
  return (length >= MIN_PASSWORD_LENGTH && length <= MAX_PASSWORD_LENGTH);
}

// --- Processing password input from RemoteXY ---
void handleRemoteXYPasswordEntry() {
  if (RemoteXY.button_confirm) {
    RemoteXY.button_confirm = 0;

    String enteredInput = RemoteXY.edit_passWord;
    String trimmedInput = enteredInput;
    trimmedInput.trim();

    // SafeBox closing operation
    if (trimmedInput.equals("#")) {
      if (isSafeBoxOpen) {
        closeSafeBoxDoor();
        // BOT_TOKEN check removed.
        if (!OWNER_CHAT_ID.isEmpty()) {
          bot.sendMessage(OWNER_CHAT_ID, "Smart Safe Box: The Safe Box was manually closed via RemoteXY.", "");
        }
        if (!SECOND_USER_CHAT_ID.isEmpty()) {
          bot.sendMessage(SECOND_USER_CHAT_ID, "Smart Safe Box: The Safe Box was manually closed via RemoteXY.", "");
        }
      } else {
        // BOT_TOKEN check removed.
        if (!OWNER_CHAT_ID.isEmpty()) {
          bot.sendMessage(OWNER_CHAT_ID, "Smart Safe Box: The Safe Box is already closed (via RemoteXY).", "");
        }
        if (!SECOND_USER_CHAT_ID.isEmpty()) {
          bot.sendMessage(SECOND_USER_CHAT_ID, "Smart Safe Box: The Safe Box is already closed (via RemoteXY).", "");
        }
      }
      RemoteXY.edit_passWord[0] = '\0';
      return;
    }

    // Password length validation
    if (!isValidPasswordLength(enteredInput.length())) {
      // BOT_TOKEN check removed.
      if (!OWNER_CHAT_ID.isEmpty()) {
        bot.sendMessage(OWNER_CHAT_ID, "Smart Safe Box: Invalid password attempt from RemoteXY (length error)!", "");
      }
      RemoteXY.edit_passWord[0] = '\0';
      return;
    }

    // Password verification
    if (enteredInput == ownerPassword) {
      deactivateAlarm();
      openSafeBoxDoor("Owner (RemoteXY)");

      // Send notification to second user only
      // BOT_TOKEN check removed.
      if (!SECOND_USER_CHAT_ID.isEmpty()) {
        bot.sendMessage(SECOND_USER_CHAT_ID, "Smart Safe Box: The Safe Box was opened by the owner (via RemoteXY).", "");
      }
      wrongAttempts = 0;
    } else if (enteredInput == secondUserPassword) {
      deactivateAlarm();
      openSafeBoxDoor("Second User (RemoteXY)");

      // Send notification to owner only
      // BOT_TOKEN check removed.
      if (!OWNER_CHAT_ID.isEmpty()) {
        bot.sendMessage(OWNER_CHAT_ID, "Smart Safe Box: The Safe Box was opened by the second user (via RemoteXY).", "");
      }
      wrongAttempts = 0;
    } else {  // If the password is incorrect
      wrongAttempts++;

      if (wrongAttempts == 1) {
        // BOT_TOKEN check removed.
        if (!OWNER_CHAT_ID.isEmpty()) bot.sendMessage(OWNER_CHAT_ID, "Smart Safe Box: Unauthorized access attempt from RemoteXY! (First incorrect attempt)", "");
        if (!SECOND_USER_CHAT_ID.isEmpty()) bot.sendMessage(SECOND_USER_CHAT_ID, "Smart Safe Box: Unauthorized access attempt from RemoteXY! (First incorrect attempt)", "");
      } else if (wrongAttempts >= MAX_WRONG_ATTEMPTS_BEFORE_ALARM) {
        activateAlarm();
        // BOT_TOKEN check removed.
        if (!OWNER_CHAT_ID.isEmpty()) bot.sendMessage(OWNER_CHAT_ID, "Smart Safe Box: The Safe Box is in danger! Alarm has been activated!", "");
        if (!SECOND_USER_CHAT_ID.isEmpty()) bot.sendMessage(SECOND_USER_CHAT_ID, "Smart Safe Box: The Safe Box is in danger! Alarm has been activated!", "");
      }
    }
    RemoteXY.edit_passWord[0] = '\0';
  }
}

// --- Processing Telegram Messages ---
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    // The following comment lines provide the Telegram ID of the person connected via Telegram
    // Serial.print("Message Chat ID: "); Serial.println(chat_id); // Remove the // at the beginning of this line
    // Serial.print("Message From (Name): "); Serial.println(from_name); // Remove the // at the beginning of this line
    // Serial.print("Message Text: "); Serial.println(text); // Remove the // at the beginning of this line

    bool isOwner = (chat_id == OWNER_CHAT_ID);
    bool isSecondUser = (chat_id == SECOND_USER_CHAT_ID);

    if (text == "/start") {
      String welcome = "Hi,Owner.\nThis is a Smart Safe Box Bot.\n ";
      welcome += "You can type /status to check the Safe Box condition.\n ";
      welcome += "You can type /alarm_off to turn off the alarm.\n ";
      welcome += "You can type /close to lock the Safe Box via Telegram.\n ";
      welcome += "You can type /change\_password \<new\_password> to change your password.\n ";
      // BOT_TOKEN check removed.
      bot.sendMessage(chat_id, welcome, "");
    } else if (text == "/status") {
      String statusMessage = "Safe Box Status: ";
      statusMessage += isSafeBoxOpen ? "OPEN." : "CLOSE.";
      statusMessage += isAlarmActive ? " Alarm Active!" : " Alarm Inactive.";
      // BOT_TOKEN check removed.
      bot.sendMessage(chat_id, statusMessage, "");
    } else if (text == "/alarm_off") {
      // AUTHORIZATION CHECK ADDED
      if (isOwner || isSecondUser) {  // If the message sender is the owner or the second user
        if (isAlarmActive) {
          deactivateAlarm();
          bot.sendMessage(chat_id, "The alarm was turned off via Telegram.", "");

          String otherUserChatId = "";
          if (isOwner && !SECOND_USER_CHAT_ID.isEmpty()) {
            otherUserChatId = SECOND_USER_CHAT_ID;
          } else if (isSecondUser && !OWNER_CHAT_ID.isEmpty()) {
            otherUserChatId = OWNER_CHAT_ID;
          }

          if (!otherUserChatId.isEmpty()) {
            bot.sendMessage(otherUserChatId, "Smart Safe Box: The alarm was turned off by " + from_name + "", "");
          }
        } else {
          bot.sendMessage(chat_id, "There is no active alarm at the moment.", "");
        }
      } else {  // If the user is unauthorized
        bot.sendMessage(chat_id, "You do not have permission to use this command or your Chat ID is not recognized.", "");
      }
    } else if (text == "/close") {
      //AUTHORIZATION CHECK ADDED
      if (isOwner || isSecondUser) {  // If the message sender is the owner or the second user
        if (isSafeBoxOpen) {
          closeSafeBoxDoor();

          bot.sendMessage(chat_id, "Smart Safe Box: The Safe Box was closed by you via Telegram.", "");

          String otherUserChatId = "";
          if (isOwner && !SECOND_USER_CHAT_ID.isEmpty()) {
            otherUserChatId = SECOND_USER_CHAT_ID;
          } else if (isSecondUser && !OWNER_CHAT_ID.isEmpty()) {
            otherUserChatId = OWNER_CHAT_ID;
          }

          if (!otherUserChatId.isEmpty()) {
            bot.sendMessage(otherUserChatId, "Smart Safe Box: The Safe Box was closed by " + from_name + " via Telegram.", "");
          }
        } else {
          bot.sendMessage(chat_id, "The Safe Box is already closed.", "");
        }
      } else {  // If the user is unauthorized
        bot.sendMessage(chat_id, "You do not have permission to use this command or your Chat ID is not recognized.", "");
      }
    } else if (text.startsWith("/change_password ")) {  // Password must be entered in the format change_password new_password
      String newPassword = text.substring(17);

      if (!isValidPasswordLength(newPassword.length())) {
        // BOT_TOKEN check removed.
        bot.sendMessage(chat_id, "Error: The new password must be between " + String(MIN_PASSWORD_LENGTH) + " and " + String(MAX_PASSWORD_LENGTH) + " characters long.", "");
        return;
      }

      if (isOwner) {
        ownerPassword = newPassword;
        savePasswords();
        // BOT_TOKEN check removed.
        bot.sendMessage(chat_id, "Your password has been successfully changed:" + newPassword, "");
        // BOT_TOKEN check removed.
        if (!SECOND_USER_CHAT_ID.isEmpty()) {
          bot.sendMessage(SECOND_USER_CHAT_ID, "Smart Safe Box: changed their own password:" + from_name + "", "");
        }
      } else if (isSecondUser) {
        secondUserPassword = newPassword;
        savePasswords();
        // BOT_TOKEN check removed.
        bot.sendMessage(chat_id, "Your password has been successfully changed:" + newPassword, "");
        // BOT_TOKEN check removed.
        if (!OWNER_CHAT_ID.isEmpty()) {
          bot.sendMessage(OWNER_CHAT_ID, "Smart Safe Box: changed their own password:" + from_name + "", "");
        }
      } else {
        // BOT_TOKEN check removed.
        bot.sendMessage(chat_id, "You do not have permission to use this command or your Chat ID is not recognized.", "");
      }
    } else {
      // BOT_TOKEN check removed.
      bot.sendMessage(chat_id, "I didn’t understand. You can type /start to get help.", "");
    }
  }
}

// --- Servo Motor Control Functions ---
void openSafeBoxDoor(const String& openedBy) {
  safeBoxServo.write(110);
  isSafeBoxOpen = true;
  safeBoxOpenTime = millis();
}

void closeSafeBoxDoor() {
  safeBoxServo.write(0);
  isSafeBoxOpen = false;
}

// --- Alarm Management Functions ---
void activateAlarm() {
  digitalWrite(BUZZER_PIN, HIGH);
  isAlarmActive = true;
}

void deactivateAlarm() {
  digitalWrite(BUZZER_PIN, LOW);
  isAlarmActive = false;
  wrongAttempts = 0;
}

// --- Permanently Saving Passwords ---
void savePasswords() {
  preferences.putString("owner_Password", ownerPassword);
  preferences.putString("secondUser_Password", secondUserPassword);
}

// --- Loading Passwords from Persistent Memory ---
void loadPasswords() {
  ownerPassword = preferences.getString("owner_Password", "");
  secondUserPassword = preferences.getString("secondUser_Password", "");
}
