function main() {
  const input1 = document.querySelector('.mdl-input:nth-child(1)');
  const input2 = document.querySelector('.mdl-input:nth-child(2)');
  
  const ws = new WebSocket(`ws://${location.host}/ws`);
  
  ws.addEventListener('open', (event) => {
    console.log('WebSocket connection opened:', event);

    const sendPaddedMessage = () => {
      const paddedInput1 = input1.value.padEnd(16, ' ');
      const paddedInput2 = input2.value.padEnd(16, ' ');
      const message = paddedInput1 + paddedInput2;
      const jsonMessage = JSON.stringify({ disp_str: message });
      ws.send(jsonMessage);
    };

    input1.addEventListener('input', sendPaddedMessage);
    input2.addEventListener('input', sendPaddedMessage);
  });

  ws.addEventListener('close', (event) => {
    console.log('WebSocket connection closed:', event);
  });

  ws.addEventListener('error', (event) => {
    console.error('WebSocket error:', event);
  });
}

// Call the main function
main();