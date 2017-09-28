package com.dp5.strategy.yc;

//���Ե��ⲿ��װ�࣬���ݲ�ͬ����Ϊִ�в�ͬ�Ĳ���
public class Context {
	private Strategy strategy;
	
	public Context(Strategy strategy){
		this.strategy = strategy;
	}
	
	public void encrypt() {
		this.strategy.encrypt();
	}
}
