const int BUFFER_SIZE = 128; // Needs to be changed according to the expected message size

char receivedData[BUFFER_SIZE]; // Buffer to store received data
int dataIndex = 0;              // Index to keep track of the next available position in the buffer
int counter = 0;                // Counter variable
int flag = 0;

// Define and initialize the array of floats
const int arraySize = 6000;
float floatArray[arraySize];

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT); // Set the built-in LED pin as an output
  Serial.begin(9600);           // Start serial communication at 9600 baud rate
}

void loop()
{
  if (Serial.available() > 0)
  { // Check if there is data available to read
    if (flag == 0)
    {
      digitalWrite(LED_BUILTIN, HIGH); // Turn on the built-in LED
      flag = 1;
    }
    else
    {
      digitalWrite(LED_BUILTIN, LOW); // Turn off the built-in LED
      flag = 0;
    }

    // Read the received data and store it in the buffer
    receivedData[dataIndex++] = counter + '0'; // Convert counter to char and store
    receivedData[dataIndex++] = ' ';

    // Read all available characters and store them in the buffer
    while (Serial.available() && dataIndex < BUFFER_SIZE - 1)
    {
      receivedData[dataIndex++] = Serial.read(); // Store received character
    }
    receivedData[dataIndex] = '\0'; // Null-terminate the string in the buffer

    // Print the entire buffer
    Serial.println(receivedData);
    dataIndex = 0; // Reset the index for the next iteration
    counter += 1;
  }

  // Initialize the array with each element equal to its index
  if (counter == 0)
  {
    for (int i = 0; i < arraySize; ++i)
    {
      floatArray[i] = static_cast<float>(i);
    }
    // Print array initialization status
    Serial.println("Array initialized");
  }
}
