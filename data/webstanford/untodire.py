def read_txt(inputpath, outputpath):
    with open(outputpath, 'w') as file:
        with open(inputpath, 'r') as infile:
            # read line
            #data1 = infile.readlines()
            #print(data1)

  
            # read line and word
            i=0
            maxmum = 0
            for line in infile:
                data_line = line.strip("\n").split()  
                data = data_line[0] + '\t' + data_line[1]+'\n'+data_line[1] + '\t' + data_line[0]+'\n'
                # print(data)
                i=i+1
                


                x= int(data_line[0])
                y = int(data_line[1])
                maxmum = max(maxmum,x,y)
                
                # file.write(data)
            print(i)
            print(maxmum)
            # read each word
            # data = data2
            # for i in range(len(data2)):
            #     for j in range(len(data2[i])):
            #         data[i][j] = str(data2[i][j])
            # print(data)

  
            # write
            # for line in data2:
            #     # data = '' + '\t'.join(str(i) for i in line) + '\n'  # 
            #     data = '' + ' '.join(str(i) for i in line) + '\n'  # 
            #     file.write(data)
  
if __name__ == "__main__":
    input_path = 'graph.txt'
    output_path = '2.txt'
    read_txt(input_path, output_path)